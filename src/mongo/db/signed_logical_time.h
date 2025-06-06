/**
 *    Copyright (C) 2018-present MongoDB, Inc.
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

#pragma once

#include "mongo/db/logical_time.h"
#include "mongo/db/time_proof_service.h"

#include <string>
#include <utility>

#include <boost/move/utility_core.hpp>
#include <boost/optional/optional.hpp>

namespace mongo {

/**
 * The SignedLogicalTime class is a pair of value i.e. time and a signature i.e. _proof
 * The class is immutable and is used to hold the cryptographically protected LogicalTime.
 */
class SignedLogicalTime {
public:
    using TimeProof = TimeProofService::TimeProof;

    SignedLogicalTime() = default;

    explicit SignedLogicalTime(LogicalTime time, long long keyId)
        : _time(std::move(time)), _keyId(keyId) {}

    explicit SignedLogicalTime(LogicalTime time, TimeProof proof, long long keyId)
        : _time(std::move(time)), _proof(std::move(proof)), _keyId(keyId) {}

    LogicalTime getTime() const {
        return _time;
    }

    boost::optional<TimeProof> getProof() const {
        return _proof;
    }

    long long getKeyId() const {
        return _keyId;
    }

    std::string toString() const;

    static const SignedLogicalTime kUninitialized;

private:
    LogicalTime _time;
    boost::optional<TimeProof> _proof;
    long long _keyId{0};
};

}  // namespace mongo
