q::Queue;
q->ToDevice(eth2);
FromDevice(eth2) -> Print -> q;
FromDevice(eth3) -> Print -> q;
