package ir.bigandsmall.hiddevice;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity {

    private static final String ACTION_USB_PERMISSION = "ir.bigandsmall.hiddevice.USB";
    private static final String Tag = "ir.bigandsmall";

    public static Intent intent = null;
    public static PendingIntent permissionIntent;
    public UsbManager manager;
    public static  UsbDevice deviceSepronik = null;
    private UsbDeviceConnection connectionSepronik = null;
    private UsbEndpoint mEndpointIn=null;
    private UsbEndpoint mEndpointOut=null;
    private D2ccDevice d2ccDevice;
    private D2ccManager d2ccManager;
    private int mInterfaceID;

    Handler mHandler = new Handler();

    private static final String VENDORID="1027";
    //private static final String PRODUCTID="24577";        //FT245
    //private static final String PRODUCTID="24607";        //FT601
    private static final String PRODUCTID="24596";      //FT232

    private boolean forceClaim = true;

    public Button btn_open;
    public Button btn_read;
    public Button btn_write;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        //boolean isRoot=upgradeRootPermission(getPackageCodePath());
        //System.out.println(isRoot?"是root":"GG了");


        intent = getIntent();
        btn_open=(Button)findViewById(R.id.button_open);
        btn_open.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                OpenDeviceClick(v);
            }
        });
        btn_read=(Button)findViewById(R.id.button_read);
        btn_read.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                ReadMemClick(v);
            }
        });
        btn_write=(Button)findViewById(R.id.button_write);
        btn_write.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                WriteMemClick(v);
            }
        });


//        byte [] a=new byte[]{1,2,3,4,5};
//        byte [] b=new byte[]{5,4,3,2,1};
//        d2ccDevice.ArrayTest(a,b);
//        System.out.println("最终B="+b[0]);
        D2ccManager.getInstance(this);
//        manager = (UsbManager) getSystemService(Context.USB_SERVICE);
//        permissionIntent = PendingIntent.getBroadcast (this, 0, new Intent(ACTION_USB_PERMISSION), 0);
//        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
//        registerReceiver(mUsbReceiver,filter);    //注册USB广播的接收

//        mHandler.postDelayed(runnable,1000);
    }

    public void WriteMemClick(View v) {
//        byte [] data=new byte[4000];
//        for (int i = 0; i < 3999; i++) {
//            data[i]=(byte)(i%128);
//        }
//        thread_write.start();
        int nameRet =  D2ccManager.getInstance().getD2ccDevice().WriteMemory();
    }

    public void ReadMemClick(View v) {
//        thread_read.start();
        int nameRet =  D2ccManager.getInstance().getD2ccDevice().ReadMemory();
    }

    public void CloseDeviceClick(View v) {
        /*int nameRet =  d2ccDevice.CloseDevice();
        if(nameRet == 0) Toast.makeText(getApplicationContext(),"Close Success" ,Toast.LENGTH_SHORT).show();
        else Toast.makeText(getApplicationContext(),"Close Error="+nameRet ,Toast.LENGTH_SHORT).show();*/
    }

    public void OpenDeviceClick(View v) {
        D2ccManager.getInstance().OpenDevice();
//        deviceSepronik = null;
//        System.out.println("按钮点击！！！");
//        UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
//        if(device == null) {
//            //final UsbManager manager = (UsbManager)getSystemService(Context.USB_SERVICE);   //获取USB manager实例
//            final HashMap<String, UsbDevice> usb_device_list = manager.getDeviceList(); //获取USB设备列表
//            for(final String desc : usb_device_list.keySet()) { //遍历整个hash表
//                final UsbDevice candidate = usb_device_list.get(desc);
//                System.out.println("VID="+candidate.getVendorId()+"  PID="+candidate.getProductId());
//                if(String.valueOf(candidate.getVendorId()).equals(VENDORID)&&String.valueOf(candidate.getProductId()).equals(PRODUCTID)) {  //查看USB设备信息是否正确
//                    deviceSepronik = candidate;
//                    System.out.println("发现设备");
//                }
//            }
//            if(deviceSepronik != null) manager.requestPermission(deviceSepronik, permissionIntent); //申请访问USB权限
//        }
    }

    public final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            //System.out.println("检测到USB了！！！！！！");
            if(ACTION_USB_PERMISSION.equals(action)) {
                synchronized(this) {
                    if(intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        if(deviceSepronik != null) {
                            final UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);//获取USB manager实例
                            if (deviceSepronik != null && !manager.hasPermission(deviceSepronik)) {
                                manager.requestPermission(deviceSepronik, permissionIntent);
                                System.out.println("manager.hasPermission(deviceSepronik)没过");
                                return;
                            }
                            //获取interface
                            System.out.println("共有interface"+deviceSepronik.getInterfaceCount());
                            UsbInterface intf = deviceSepronik.getInterface(0);
                            System.out.println("接口是"+intf);

                            for (int i = 0; i < intf.getEndpointCount(); i++) {
                                UsbEndpoint ep = intf.getEndpoint(i);
                                if (ep.getType() == UsbConstants.USB_ENDPOINT_XFER_BULK) {
                                    if((ep.getDirection() == UsbConstants.USB_DIR_IN)&&(ep.getDirection() == UsbConstants.USB_DIR_OUT)){

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
                            connectionSepronik = manager.openDevice(deviceSepronik);
                            if (connectionSepronik != null&&connectionSepronik.claimInterface(intf, true)) {
                                connectionSepronik.controlTransfer(64, 11, 64<<8, mInterfaceID, (byte[])null, 0, 0);    //将FTDI芯片配置到同步fifo245模式
                                connectionSepronik.controlTransfer(64, 6, 0, mInterfaceID, (byte[])null, 0, 0);
                                connectionSepronik.controlTransfer(64, 7, 0, mInterfaceID, (byte[])null, 0, 0);
                                connectionSepronik.controlTransfer(64, 9, 16, mInterfaceID, (byte[])null, 0, 0);
//                                thread.start();
//                                System.out.println("发送控制的返回值"+ret);
                                d2ccDevice.OpenDevice(connectionSepronik.getFileDescriptor(),inEndPoint,outEndPoint);    //在native代码中打开USB设备文件
                            }
                            //connectionSepronik.bulkTransfer(mEndpointIn,new byte[1000],1000,1000);

                        }//end: if(deviceSepronik != null)
                    }
                }
            }   //end if(ACTION_USB_PERMISSION.equals(action))
        }
    };  //end： 广播内容
//    Thread thread_read=new Thread(){
//        public void run() {
//            while(true){
//                System.out.println("开始读线程");
//                long start=System.currentTimeMillis();
//                int nameRet =  d2ccDevice.ReadMemory();
//
//                //System.out.println("读完"+(System.currentTimeMillis()-start));
//                //System.out.println("共读取"+nameRet);
//                try {
//                    Thread.sleep(50);
//                } catch (InterruptedException e) {
//                }
//            }
//        }
//    };
//    Thread thread_write=new Thread(){
//        public void run() {
////            System.out.println("开始写线程");
////            byte [] data =new byte[4096*2];
////            for(int i=0;i<4096*2;i++){
////                data[i]=1;
////            }
//            int nameRet =  d2ccDevice.WriteMemory();
////            for(int j=0;j<100;j++) {
////                //long start=System.currentTimeMillis();
////                //int nameRet = connectionSepronik.bulkTransfer(mEndpointOut, data,1, 4000);
////                //data[0]++;
////                int nameRet =  d2ccDevice.WriteMemory();
////                //System.out.println("写完"+(System.currentTimeMillis()-start));
////                System.out.println("第" +j+"次 共写入" + nameRet);
////                try {
////                    Thread.sleep(1000);
////                } catch (InterruptedException e) {
////                }
////            }
//
//        }
//    };

}
