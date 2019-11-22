DROP TABLE myt;
DROP EXTENSION zoit CASCADE;

CREATE EXTENSION zoit;

CREATE TABLE myt (i int4);

CREATE INDEX zoitidx ON myt USING zoit (i int4_ops);
INSERT INTO myt SELECT i FROM generate_series(1,2000) i;
CHECKPOINT;

ANALYZE;
EXPLAIN (COSTS OFF) SELECT * FROM myt WHERE i = 7;
SELECT * FROM myt WHERE i = 7;

SELECT relfilenode, relname FROM pg_class WHERE relname LIKE '%zoit%';
