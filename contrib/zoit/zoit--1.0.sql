/* contrib/zoit/zoit.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION zoit" to load this file. \quit

CREATE FUNCTION zthandler(internal)
RETURNS index_am_handler
AS 'MODULE_PATHNAME'
LANGUAGE C;

-- Access method
CREATE ACCESS METHOD zoit TYPE INDEX HANDLER zthandler;
COMMENT ON ACCESS METHOD zoit IS 'zoit index access method';

-- Opclasses

CREATE OPERATOR CLASS int4_ops
DEFAULT FOR TYPE int4 USING zoit AS
	OPERATOR	1	=(int4, int4),
	FUNCTION	1	hashint4(int4);

CREATE OPERATOR CLASS text_ops
DEFAULT FOR TYPE text USING zoit AS
	OPERATOR	1	=(text, text),
	FUNCTION	1	hashtext(text);
