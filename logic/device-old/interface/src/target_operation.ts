interface CoreOperation {
    speculative: boolean // 0=not speculative, 1=speculative/prefetch.
    effectful: boolean // 0=get/set, 1=read/write.
}

export type Operation = CoreOperation & ({data:true, instruction?:never}|{data?:never, instruction:true})
