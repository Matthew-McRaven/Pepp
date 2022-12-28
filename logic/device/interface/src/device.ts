export interface DevicePOD {
    readonly baseName: string
    readonly fullName:string
    readonly deviceID:number
    readonly compatible:string
}

export interface Device {
    baseName:()=>string
    fullName:()=>string
    deviceID:()=>number
    compatible:()=>string
}
export type NonNativeDevice = Device
export type NativeDevice = Device
