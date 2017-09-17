package ir.bigandsmall.hiddevice;

public class ClientJni {

    static {
        System.loadLibrary("d2cc");
    }
//    public native void ArrayTest(byte []in,byte [] out);


    //以下函数可以根据用户需要修改，本例中只是最简单的读写测试函数
    public native int ReadMemory();
    public native int WriteMemory();
}
