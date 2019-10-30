CREATE EXTENSION zoit;

CREATE TABLE tst (
	i	int4,
	t	text
);

INSERT INTO tst SELECT i%10, substr(md5(i::text), 1, 1) FROM generate_series(1,2000) i;
CREATE INDEX zoitidx ON tst USING zoit (i, t);

SET enable_seqscan=off;
SET enable_bitmapscan=on;
SET enable_indexscan=on;
EXPLAIN (COSTS OFF) SELECT * FROM tst WHERE i = 7;
SELECT * FROM tst WHERE i = 7;

DROP TABLE tst;
DROP EXTENSION zoit;
