FromDevice(eth5) -> Print (MAXLENGTH 100)-> Discard;

q::Queue;

q -> ToDevice(eth5);

TimedSource(DATA \<08002781b5ad 010203040506  08060001080006040002 010203040506 c0a80004 08002781b5ad c0a80004 0000 00000000 00000000 00000000 00000000>, INTERVAL 5) -> Print(ME, MAXLENGTH 100) -> q;

//TimedSource(DATA \<08002781b5ad 0000000A0B0C  08060001080006040002 0000000A0B0C c0a80004 08002781b5ad c0a80004 0000 00000000 00000000 00000000 00000000>, INTERVAL 6) -> Print(ME2, MAXLENGTH 100) -> q;
