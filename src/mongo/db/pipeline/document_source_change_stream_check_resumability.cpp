/**
 *    Copyright (C) 2021-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/db/pipeline/document_source_change_stream_check_resumability.h"

#include "mongo/base/error_codes.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/bson/bsontypes.h"
#include "mongo/bson/timestamp.h"
#include "mongo/db/exec/document_value/document_metadata_fields.h"
#include "mongo/db/exec/document_value/value_comparator.h"
#include "mongo/db/pipeline/change_stream_helpers.h"
#include "mongo/db/pipeline/document_source_change_stream.h"
#include "mongo/idl/idl_parser.h"
#include "mongo/logv2/log.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/str.h"

#include <utility>

#include <boost/optional.hpp>
#include <boost/optional/optional.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

using boost::intrusive_ptr;

#define MONGO_LOGV2_DEFAULT_COMPONENT ::mongo::logv2::LogComponent::kQuery

namespace mongo {

REGISTER_INTERNAL_DOCUMENT_SOURCE(_internalChangeStreamCheckResumability,
                                  LiteParsedDocumentSourceChangeStreamInternal::parse,
                                  DocumentSourceChangeStreamCheckResumability::createFromBson,
                                  true);
ALLOCATE_DOCUMENT_SOURCE_ID(_internalChangeStreamCheckResumability,
                            DocumentSourceChangeStreamCheckResumability::id)

// Returns ResumeStatus::kFoundToken if the document retrieved from the resumed pipeline satisfies
// the client's resume token, ResumeStatus::kCheckNextDoc if it is older than the client's token,
// and ResumeToken::kSurpassedToken if it is more recent than the client's resume token, indicating
// that we will never see the token. Return ResumeStatus::kNeedsSplit if we have found the event
// that produced the resume token, but it was split in the original stream.
DocumentSourceChangeStreamCheckResumability::ResumeStatus
DocumentSourceChangeStreamCheckResumability::compareAgainstClientResumeToken(
    const Document& eventFromResumedStream, const ResumeTokenData& tokenDataFromClient) {
    // Parse the stream doc into comprehensible ResumeTokenData.
    auto tokenDataFromResumedStream =
        ResumeToken::parse(eventFromResumedStream.metadata().getSortKey().getDocument()).getData();

    // We start the resume with a $gte query on the timestamp, so we never expect it to be lower
    // than our resume token's timestamp.
    invariant(tokenDataFromResumedStream.clusterTime >= tokenDataFromClient.clusterTime);

    // If the clusterTime differs from the client's token, this stream cannot be resumed.
    if (tokenDataFromResumedStream.clusterTime != tokenDataFromClient.clusterTime) {
        return ResumeStatus::kSurpassedToken;
    }

    // If the tokenType exceeds the client token's type, then we have passed the resume token point.
    // This can happen if the client resumes from a synthetic 'high water mark' token from another
    // shard which happens to have the same clusterTime as an actual change on this shard.
    if (tokenDataFromResumedStream.tokenType != tokenDataFromClient.tokenType) {
        return tokenDataFromResumedStream.tokenType > tokenDataFromClient.tokenType
            ? ResumeStatus::kSurpassedToken
            : ResumeStatus::kCheckNextDoc;
    }

    // If the document's 'txnIndex' sorts before that of the client token, we must keep looking.
    if (tokenDataFromResumedStream.txnOpIndex < tokenDataFromClient.txnOpIndex) {
        return ResumeStatus::kCheckNextDoc;
    } else if (tokenDataFromResumedStream.txnOpIndex > tokenDataFromClient.txnOpIndex) {
        // This could happen if the client provided a txnOpIndex of 0, yet the 0th document in the
        // applyOps was irrelevant (meaning it was an operation on a collection or DB not being
        // watched). Signal that we have read beyond the resume token.
        return ResumeStatus::kSurpassedToken;
    }

    // If 'fromInvalidate' exceeds the client's token value, then we have passed the resume point.
    if (tokenDataFromResumedStream.fromInvalidate != tokenDataFromClient.fromInvalidate) {
        return tokenDataFromResumedStream.fromInvalidate ? ResumeStatus::kSurpassedToken
                                                         : ResumeStatus::kCheckNextDoc;
    }

    // It is acceptable for the stream UUID to differ from the client's, if this is a whole-database
    // or cluster-wide stream and we are comparing operations from different shards at the same
    // clusterTime. If the stream UUID sorts after the client's, however, then the stream is not
    // resumable; we are past the point in the stream where the token should have appeared.
    if (tokenDataFromResumedStream.uuid != tokenDataFromClient.uuid) {
        return tokenDataFromResumedStream.uuid > tokenDataFromClient.uuid
            ? ResumeStatus::kSurpassedToken
            : ResumeStatus::kCheckNextDoc;
    }

    // If the eventIdentifier matches exactly, then we have found the resume point. However, this
    // event may have been split by the original stream; we must check the value of the resume
    // token's fragmentNum field to determine the correct return status.
    if (ValueComparator::kInstance.evaluate(tokenDataFromResumedStream.eventIdentifier ==
                                            tokenDataFromClient.eventIdentifier)) {
        if (tokenDataFromClient.fragmentNum && !tokenDataFromResumedStream.fragmentNum) {
            return ResumeStatus::kNeedsSplit;
        }
        if (tokenDataFromResumedStream.fragmentNum == tokenDataFromClient.fragmentNum) {
            return ResumeStatus::kFoundToken;
        }
        return tokenDataFromResumedStream.fragmentNum > tokenDataFromClient.fragmentNum
            ? ResumeStatus::kSurpassedToken
            : ResumeStatus::kCheckNextDoc;
    }

    // At this point, we know that the tokens differ only by eventIdentifier. The status we return
    // will depend on whether the stream token is logically before or after the client token. If the
    // latter, then we will never see the resume token and the stream cannot be resumed.
    return ValueComparator::kInstance.evaluate(tokenDataFromResumedStream.eventIdentifier >
                                               tokenDataFromClient.eventIdentifier)
        ? ResumeStatus::kSurpassedToken
        : ResumeStatus::kCheckNextDoc;
}

DocumentSourceChangeStreamCheckResumability::DocumentSourceChangeStreamCheckResumability(
    const intrusive_ptr<ExpressionContext>& expCtx, ResumeTokenData token)
    : DocumentSourceInternalChangeStreamStage(getSourceName(), expCtx),
      _tokenFromClient(std::move(token)) {}

intrusive_ptr<DocumentSourceChangeStreamCheckResumability>
DocumentSourceChangeStreamCheckResumability::create(const intrusive_ptr<ExpressionContext>& expCtx,
                                                    const DocumentSourceChangeStreamSpec& spec) {
    auto resumeToken = change_stream::resolveResumeTokenFromSpec(expCtx, spec);
    return new DocumentSourceChangeStreamCheckResumability(expCtx, std::move(resumeToken));
}

intrusive_ptr<DocumentSourceChangeStreamCheckResumability>
DocumentSourceChangeStreamCheckResumability::createFromBson(
    BSONElement spec, const boost::intrusive_ptr<ExpressionContext>& expCtx) {
    uassert(5467603,
            str::stream() << "the '" << kStageName << "' object spec must be an object",
            spec.type() == BSONType::object);

    auto parsed = DocumentSourceChangeStreamCheckResumabilitySpec::parse(
        IDLParserContext("DocumentSourceChangeStreamCheckResumabilitySpec"), spec.embeddedObject());
    return new DocumentSourceChangeStreamCheckResumability(expCtx,
                                                           parsed.getResumeToken().getData());
}

const char* DocumentSourceChangeStreamCheckResumability::getSourceName() const {
    return kStageName.data();
}

DocumentSource::GetNextResult DocumentSourceChangeStreamCheckResumability::doGetNext() {
    if (_resumeStatus == ResumeStatus::kSurpassedToken) {
        return pSource->getNext();
    }

    while (_resumeStatus != ResumeStatus::kSurpassedToken) {
        // The underlying oplog scan will throw OplogQueryMinTsMissing if the minTs in the change
        // stream filter has fallen off the oplog. Catch this and throw a more explanatory error.
        auto nextInput = [this]() {
            try {
                return pSource->getNext();
            } catch (const ExceptionFor<ErrorCodes::OplogQueryMinTsMissing>& ex) {
                LOGV2_ERROR(6663107,
                            "Resume of change stream was not possible",
                            "reason"_attr = ex.reason());
                uasserted(ErrorCodes::ChangeStreamHistoryLost,
                          "Resume of change stream was not possible, as the resume point may no "
                          "longer be in the oplog.");
            }
        }();

        // If we hit EOF, return it immediately.
        if (!nextInput.isAdvanced()) {
            return nextInput;
        }

        // Determine whether the current event sorts before, equal to or after the resume token.
        _resumeStatus =
            DocumentSourceChangeStreamCheckResumability::compareAgainstClientResumeToken(
                nextInput.getDocument(), _tokenFromClient);
        switch (_resumeStatus) {
            case ResumeStatus::kCheckNextDoc:
                // If the result was kCheckNextDoc, we are resumable but must swallow this event.
                continue;
            case ResumeStatus::kNeedsSplit:
                // If the result was kNeedsSplit, we found a resume token which matches the client's
                // except for the splitNum attribute. Allow this document to pass through so that
                // the split stage can regenerate the original fragments and their resume tokens.
                return nextInput;
            case ResumeStatus::kSurpassedToken:
                // In this case the resume token wasn't found; it may be on another shard. However,
                // since the oplog scan did not throw, we know that we are resumable. Fall through
                // into the following case and return the document.
                return nextInput;
            case ResumeStatus::kFoundToken:
                // We found the actual token! Return the doc so DSEnsureResumeTokenPresent sees it.
                return nextInput;
        }
    }
    MONGO_UNREACHABLE;
}

Value DocumentSourceChangeStreamCheckResumability::doSerialize(
    const SerializationOptions& opts) const {
    BSONObjBuilder builder;
    if (opts.isSerializingForExplain()) {
        BSONObjBuilder sub(builder.subobjStart(DocumentSourceChangeStream::kStageName));
        sub.append("stage"_sd, kStageName);
        sub << "resumeToken"_sd << Value(ResumeToken(_tokenFromClient).toDocument(opts));
        sub.done();
    } else {
        builder.append(
            kStageName,
            DocumentSourceChangeStreamCheckResumabilitySpec(ResumeToken(_tokenFromClient))
                .toBSON());
    }
    return Value(builder.obj());
}

}  // namespace mongo
