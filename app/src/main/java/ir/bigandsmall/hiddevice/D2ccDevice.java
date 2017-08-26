package ir.bigandsmall.hiddevice;

public class D2ccDevice {

    static {
        System.loadLibrary("d2cc");
    }
    //从D2ccManager.java这种调用，作用为传递打开USB设备文件的描述信息。用户不需要修改
    public native void OpenDevice(int fd, int endPointIn,int endPointOut);
    public native void CloseDevice();


    //以下函数可以根据用户需要修改，本例中只是最简单的读写测试函数
    public native int ReadMemory();
    public native int WriteMemory();
}
