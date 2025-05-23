## Data Correctness tests:

|Component|Sub-component|Existing tests|
|---|---|---|
|Checkpoint||[main.c](../test/csuite/wt3363_checkpoint_op_races/main.c)
## Functional Correctness tests:

|Component|Sub-component|Existing tests|
|---|---|---|
|Aggregated Time Windows||[test_rollback_to_stable18.py](../test/suite/test_rollback_to_stable18.py)
|Backup||[test_txn04.py](../test/suite/test_txn04.py)
|Backup|Cursors|[test_backup01.py](../test/suite/test_backup01.py), [test_backup11.py](../test/suite/test_backup11.py)
|Checkpoint||[test_checkpoint02.py](../test/suite/test_checkpoint02.py)
|Checkpoint|Checkpoint Cleanup|[test_cc02.py](../test/suite/test_cc02.py)
|Checkpoint|History Store|[test_checkpoint03.py](../test/suite/test_checkpoint03.py)
|Checkpoint|Metadata|[test_checkpoint_snapshot01.py](../test/suite/test_checkpoint_snapshot01.py)
|Checkpoint|Recovery|[test_bug029.py](../test/suite/test_bug029.py)
|Compression||[test_dictionary01.py](../test/suite/test_dictionary01.py), [test_dictionary02.py](../test/suite/test_dictionary02.py), [test_dictionary03.py](../test/suite/test_dictionary03.py), [test_dictionary04.py](../test/suite/test_dictionary04.py)
|Config Api||[test_base02.py](../test/suite/test_base02.py), [test_config02.py](../test/suite/test_config02.py)
|Connection Api||[test_version.py](../test/suite/test_version.py)
|Connection Api|Reconfigure|[test_reconfig01.py](../test/suite/test_reconfig01.py), [test_reconfig02.py](../test/suite/test_reconfig02.py)
|Connection Api|Turtle File|[test_bug024.py](../test/suite/test_bug024.py)
|Connection Api|Wiredtiger Open|[test_config02.py](../test/suite/test_config02.py)
|Cursors|Prepare|[test_prepare_cursor01.py](../test/suite/test_prepare_cursor01.py)
|Cursors|Reconfigure|[test_cursor06.py](../test/suite/test_cursor06.py)
|Cursors|Search|[test_bug008.py](../test/suite/test_bug008.py)
|Cursors|Search Near|[test_bug008.py](../test/suite/test_bug008.py)
|Cursors|Statistics|[test_stat03.py](../test/suite/test_stat03.py)
|Encryption||[test_encrypt02.py](../test/suite/test_encrypt02.py)
|Eviction|Prepare|[test_prepare12.py](../test/suite/test_prepare12.py)
|History Store||[test_hs09.py](../test/suite/test_hs09.py)
|History Store|Eviction Checkpoint Interaction|[test_hs15.py](../test/suite/test_hs15.py)
|Indexes||[test_schema03.py](../test/suite/test_schema03.py)
|Indexes|Search Near|[test_index02.py](../test/suite/test_index02.py)
|Prepare||[test_prepare09.py](../test/suite/test_prepare09.py)
|Reconciliation|Overflow Keys|[test_bug004.py](../test/suite/test_bug004.py)
|Recovery||[test_txn04.py](../test/suite/test_txn04.py)
|Recovery|Log Files|[test_txn19.py](../test/suite/test_txn19.py)
|Rollback To Stable||[test_checkpoint_snapshot03.py](../test/suite/test_checkpoint_snapshot03.py), [test_rollback_to_stable16.py](../test/suite/test_rollback_to_stable16.py), [test_rollback_to_stable18.py](../test/suite/test_rollback_to_stable18.py)
|Salvage|Prepare|[test_prepare_hs03.py](../test/suite/test_prepare_hs03.py)
|Schema Api||[test_schema03.py](../test/suite/test_schema03.py)
|Session Api||[test_strerror01.py](../test/suite/test_strerror01.py)
|Session Api|Reconfigure|[test_reconfig04.py](../test/suite/test_reconfig04.py), [test_reconfig05.py](../test/suite/test_reconfig05.py)
|Session Api|Verify|[test_bug005.py](../test/suite/test_bug005.py)
|Statistics||[test_stat_log02.py](../test/suite/test_stat_log02.py)
|Tiered Storage|Checkpoint|[test_tiered08.py](../test/suite/test_tiered08.py)
|Tiered Storage|Flush Tier|[test_tiered08.py](../test/suite/test_tiered08.py)
|Transactions||[test_txn01.py](../test/suite/test_txn01.py)
|Truncate||[test_truncate01.py](../test/suite/test_truncate01.py)
|Truncate|Prepare|[test_prepare13.py](../test/suite/test_prepare13.py)
|Verify|Prepare|[test_prepare_hs03.py](../test/suite/test_prepare_hs03.py), [test_timestamp18.py](../test/suite/test_timestamp18.py)
|Wt Util||[test_backup01.py](../test/suite/test_backup01.py), [test_dump.py](../test/suite/test_dump.py), [test_dump01.py](../test/suite/test_dump01.py), [test_dump02.py](../test/suite/test_dump02.py), [test_dump03.py](../test/suite/test_dump03.py), [test_util11.py](../test/suite/test_util11.py)
