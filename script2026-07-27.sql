insert into "MY_EVENT" ("EVN_ID", "EVN_DATE", "EVN_INFO", "EVN_HIVE_ID") values (1, '2026-07-26', 'Вечером был возможный напад на улей', '2');

CREATE INDEX "idx_MY_CURE_" on "MY_CURE" ("CUR_HIVE_ID");
CREATE INDEX "idx_MY_CURE_2" on "MY_CURE" ("CUR_DATE");

CREATE INDEX "idx_MY_EVENT_" on "MY_EVENT" ("EVN_HIVE_ID");
CREATE INDEX "idx_MY_EVENT_2" on "MY_EVENT" ("EVN_DATE");

CREATE INDEX "idx_MY_INSPECT_" on "MY_INSPECT" ("INS_HIVE_ID");
CREATE INDEX "idx_MY_INSPECT_2" on "MY_INSPECT" ("INS_DATE");
