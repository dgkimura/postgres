DROP TABLE tst;
DROP EXTENSION zoit;

CREATE EXTENSION zoit;

CREATE TABLE tst (
	i	int4,
	t	text
);

INSERT INTO tst SELECT i%10, substr(md5(i::text), 1, 1) FROM generate_series(1,2000) i;
CREATE INDEX zoitidx ON tst USING zoit (i, t);
CHECKPOINT;

INSERT INTO tst values(42, 'narf');

SET enable_seqscan=off;
SET enable_bitmapscan=on;
SET enable_indexscan=on;
ANALYZE;
EXPLAIN (COSTS OFF) SELECT * FROM tst WHERE i = 7;
SELECT * FROM tst WHERE i = 7;

SELECT relfilenode, relname FROM pg_class WHERE relname LIKE '%zoit%';
