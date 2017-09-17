package ir.bigandsmall.hiddevice;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;

import java.util.HashMap;

/**
 * Created by xingkong on 2017/3/13.
 */

public class D2ccManager {

    Context context;
    Intent intent;
    private static final String ACTION_USB_PERMISSION = "ir.bigandsmall.hiddevice.USB";

    //操作USB相关的结构体
    public UsbManager manager;
    public static UsbDevice deviceD2cc = null;
    private UsbDeviceConnection connection = null;
    private UsbInterface mUsbInterface;
    private UsbEndpoint mEndpointIn=null;
    private UsbEndpoint mEndpointOut=null;
    private ClientJni clientJni;
    private int mInterfaceID;

    public static PendingIntent permissionIntent;

    private boolean mIsOpen;    //USB设备是否已打开
    private static D2ccManager mInstance;

    //支持的设备列表
    private static final String VENDORID="1027";
    //private static final String PRODUCTID="24577";        //FT245
    //private static final String PRODUCTID="24607";        //FT601
//    private static final String PRODUCTID="24596";      //FT232


    //各种静态常数，从d2xx照抄
    public static final byte FT_DATA_BITS_7 = 7;
    public static final byte FT_DATA_BITS_8 = 8;
    public static final byte FT_STOP_BITS_1 = 0;
    public static final byte FT_STOP_BITS_2 = 2;
    public static final byte FT_PARITY_NONE = 0;
    public static final byte FT_PARITY_ODD = 1;
    public static final byte FT_PARITY_EVEN = 2;
    public static final byte FT_PARITY_MARK = 3;
    public static final byte FT_PARITY_SPACE = 4;
    public static final short FT_FLOW_NONE = 0;
    public static final short FT_FLOW_RTS_CTS = 256;
    public static final short FT_FLOW_DTR_DSR = 512;
    public static final short FT_FLOW_XON_XOFF = 1024;
    public static final byte FT_PURGE_RX = 1;
    public static final byte FT_PURGE_TX = 2;
    public static final byte FT_CTS = 16;
    public static final byte FT_DSR = 32;
    public static final byte FT_RI = 64;
    public static final byte FT_DCD = -128;
    public static final byte FT_OE = 2;
    public static final byte FT_PE = 4;
    public static final byte FT_FE = 8;
    public static final byte FT_BI = 16;
    public static final byte FT_EVENT_RXCHAR = 1;
    public static final byte FT_EVENT_MODEM_STATUS = 2;
    public static final byte FT_EVENT_LINE_STATUS = 4;
    public static final byte FT_EVENT_REMOVED = 8;
    public static final byte FT_FLAGS_OPENED = 1;
    public static final byte FT_FLAGS_HI_SPEED = 2;
    public static final int FT_DEVICE_232B = 0;
    public static final int FT_DEVICE_8U232AM = 1;
    public static final int FT_DEVICE_UNKNOWN = 3;
    public static final int FT_DEVICE_2232 = 4;
    public static final int FT_DEVICE_232R = 5;
    public static final int FT_DEVICE_245R = 5;
    public static final int FT_DEVICE_2232H = 6;
    public static final int FT_DEVICE_4232H = 7;
    public static final int FT_DEVICE_232H = 8;
    public static final int FT_DEVICE_X_SERIES = 9;
    public static final int FT_DEVICE_4222_0 = 10;
    public static final int FT_DEVICE_4222_1_2 = 11;
    public static final int FT_DEVICE_4222_3 = 12;
    public static final byte FT_BITMODE_RESET = 0;
    public static final byte FT_BITMODE_ASYNC_BITBANG = 1;
    public static final byte FT_BITMODE_MPSSE = 2;
    public static final byte FT_BITMODE_SYNC_BITBANG = 4;
    public static final byte FT_BITMODE_MCU_HOST = 8;
    public static final byte FT_BITMODE_FAST_SERIAL = 16;
    public static final byte FT_BITMODE_CBUS_BITBANG = 32;
    public static final byte FT_BITMODE_SYNC_FIFO = 64;
    public static final int FTDI_BREAK_OFF = 0;
    public static final int FTDI_BREAK_ON = 16384;

    private D2ccManager(Context context1){
        context=context1;
        //intent=context.getIntent();
        manager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        permissionIntent = PendingIntent.getBroadcast (context, 0, new Intent(ACTION_USB_PERMISSION), 0);
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        filter.addAction("android.hardware.usb.action.USB_DEVICE_ATTACHED");
        filter.addAction("android.hardware.usb.action.USB_DEVICE_DETACHED");
        context.getApplicationContext().registerReceiver(mUsbReceiver,filter);    //注册USB广播的接收
        clientJni = new ClientJni();
    }
    /*  获取D2ccManager类的静态实例。第一次调用系统会自动创建实例，
    *   之后再调用就会返回第一次创建的实例
    *   @return:静态实例mInstance
    *   @context1:由Activity传来的Context对象
    * */
    public static D2ccManager getInstance(Context context1){
        if(mInstance==null){
            synchronized (D2ccManager.class){
                if(mInstance==null){
                    mInstance=new D2ccManager(context1);
                }
            }
        }
        return mInstance;
    }

    public static D2ccManager getInstance(){
        return mInstance;
    }
    /*  打开USB设备：若已经存在打开的设备此函数胡就啥也不做
    *   如果没有已打开的设备，函数会遍历Android系统接入的所有USB设备，对比设备的PID；VID是否符合要求
    *   若符合要求就向Android系统申请该USB设备的访问权限。系统发送android.hardware.usb.action.USB_DEVICE_ATTACHED广播
    *   @return:静态实例mInstance
    *   @context1:由Activity传来的Context对象
    * */
    public void OpenDevice(){
        deviceD2cc= null;
        //UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        //if(device == null) {
        if(!isOpen()) {
            //final UsbManager manager = (UsbManager)getSystemService(Context.USB_SERVICE);   //获取USB manager实例
            final HashMap<String, UsbDevice> usb_device_list = manager.getDeviceList(); //获取USB设备列表
            for(final String desc : usb_device_list.keySet()) { //遍历整个hash表
                final UsbDevice candidate = usb_device_list.get(desc);
//                System.out.println("VID="+candidate.getVendorId()+"  PID="+candidate.getProductId());
                if(String.valueOf(candidate.getVendorId()).equals(VENDORID)/*&&String.valueOf(candidate.getProductId()).equals(PRODUCTID)*/) {  //查看USB设备信息是否正确
                    deviceD2cc = candidate;
                    System.out.println("发现设备");
                    break;
                }
            }
            if(deviceD2cc != null) manager.requestPermission(deviceD2cc, permissionIntent); //申请访问USB权限
        }
    }

    //USB广播，在USB插入或拔出时由系统调用
    public final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            System.out.println("检测到USB了！！！！！！");
            //if(ACTION_USB_PERMISSION.equals(action)) {
            if(ACTION_USB_PERMISSION.equals(action)){ //USB插入事件
                if(intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                    if(deviceD2cc == null) {
                        return;
                    }
                    final UsbManager manager = (UsbManager) context.getSystemService(Context.USB_SERVICE);//获取USB manager实例
                    if (deviceD2cc != null && !manager.hasPermission(deviceD2cc)) {
                        manager.requestPermission(deviceD2cc, permissionIntent);
                        System.out.println("manager.hasPermission(deviceSepronik)没过");
                        return;
                    }
                    //获取interface
                    //System.out.println("共有interface"+deviceD2cc.getInterfaceCount());
                    for (int j=0;j<deviceD2cc.getInterfaceCount();j++){     //遍历所有interface
                        UsbInterface intf = deviceD2cc.getInterface(j);
                        //System.out.println("接口是"+intf);
                        for (int i = 0; i < intf.getEndpointCount(); i++) {     //遍历所有endpoint
                            UsbEndpoint ep = intf.getEndpoint(i);
                            if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK) {
                                if((ep.getDirection() == UsbConstants.USB_DIR_IN)&&(ep.getDirection() == UsbConstants.USB_DIR_OUT)){
                                    //双向端口，不知如何处理
                                }
                                else if (ep.getDirection() == UsbConstants.USB_DIR_IN) {
                                    mEndpointIn = ep;
                                    System.out.println("输入endpoint"+ep);
                                } else if (ep.getDirection() == UsbConstants.USB_DIR_OUT){
                                    mEndpointOut = ep;
                                    System.out.println("输出endpoint"+ep);
                                }
                            }
                        }
                        mInterfaceID=intf.getId()+1;
                        int inEndPoint = mEndpointIn.getAddress();   //获取endpoint
                        int outEndPoint = mEndpointOut.getAddress();
                        System.out.println("in  "+inEndPoint+"   out  "+outEndPoint);
                        //打开USB设备
                        connection = manager.openDevice(deviceD2cc);
                        if (connection != null&&connection.claimInterface(intf, true)) {
                            mIsOpen=true;
                            setBitMode((byte) 0, FT_BITMODE_SYNC_FIFO);    //配置USB工作在同步FIFO模式
                            System.out.println("准备进入C++ open函数");
                            OpenDevice(connection.getFileDescriptor(),inEndPoint,outEndPoint);    //在native代码中打开USB设备文件

                            break;  //找到可读写的节点后就跳出循环
                        }
                    }

                }

            }   //end if(ACTION_USB_PERMISSION.equals(action))
            if("android.hardware.usb.action.USB_DEVICE_DETACHED".equals(action)) {
                CloseDevice();
                mIsOpen=false;
            }
        }
    };  //end： 广播内容


    static {
        System.loadLibrary("d2cc");
    }
    //从D2ccManager.java这种调用，作用为传递打开USB设备文件的描述信息。用户不需要修改
    public native void OpenDevice(int fd, int endPointIn,int endPointOut);
    public native void CloseDevice();

    //USB设备是否已打开
    public synchronized boolean isOpen() {
        return this.mIsOpen;
    }
    public ClientJni getClientJni(){
        return clientJni;
    }

    UsbDeviceConnection getConnection() {
        return this.connection;
    }
    //设置USB接收模式
    public boolean setBitMode(byte mask, byte bitMode) {
        boolean boolStatus = false;
        if(!this.isOpen()) {
            return boolStatus;
        }
        else {
            int wValue = bitMode << 8;
            wValue |= mask & 255;
            int status = this.getConnection().controlTransfer(64, 11, wValue, this.mInterfaceID, (byte[])null, 0, 0);
            if(status == 0) {
                boolStatus = true;
            }
            return boolStatus;
        }
    }
    /*
    Specifies the event character and error replacement characters for the device to use.
    When the device detects an event character being received,
    this will trigger an IN to the USB Host regardless of the number of bytes in the device's buffer or the latency timer value.
    When the device detects an error (D2xxManager.FT_OE, D2xxManager.FT_PE, D2xxManager.FT_FE or D2xxManager.FT_BI),
    the error character will be inserted in to the data stream to the USB host.
    * */
    public boolean setChars(byte eventChar, byte eventCharEnable, byte errorChar, byte errorCharEnable) {
        boolean rc = false;
        if(!this.isOpen()) {
            return rc;
        } else {
            int wValue = eventChar & 255;
            if(eventCharEnable != 0) {
                wValue |= 256;
            }
            int status = this.getConnection().controlTransfer(64, 6, wValue, this.mInterfaceID, (byte[])null, 0, 0);
            if(status != 0) {
                return rc;
            } else {
                wValue = errorChar & 255;
                if(errorCharEnable > 0) {
                    wValue |= 256;
                }
                status = this.getConnection().controlTransfer(64, 7, wValue, this.mInterfaceID, (byte[])null, 0, 0);
                if(status == 0) {
                    rc = true;
                }
                return rc;
            }
        }
    }
/*
* This method allows the latency timer value for the device to be specified.
* The latency timer is the mechanism that returns short packets to the USB host.
* The default value is 16ms.
* */
    public boolean setLatencyTimer(byte latency) {
        boolean rc = false;
        int wValue = latency & 255;
        if(!this.isOpen()) {
            return rc;
        } else {
            int status = this.getConnection().controlTransfer(64, 9, wValue, this.mInterfaceID, (byte[])null, 0, 0);
            if(status == 0) {
                rc = true;
            } else {
                rc = false;
            }

            return rc;
        }
    }


}
