import { Target } from './target';

export interface Initiator {
    setTarget: (target:Target)=>void;
}

export interface SplitInitiator {
    setITarget: (target:Target)=>void;
    setDTarget: (target:Target)=>void;
}
