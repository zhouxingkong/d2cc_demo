package ir.bigandsmall.hiddevice;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;

public class MainActivity extends Activity {

    public static Intent intent = null;

    Handler mHandler = new Handler();

    public Button btn_open;
    public Button btn_read;
    public Button btn_write;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

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
    }

    public void WriteMemClick(View v) {
        int nameRet =  D2ccManager.getInstance().getClientJni().WriteMemory();
    }

    public void ReadMemClick(View v) {
        int nameRet =  D2ccManager.getInstance().getClientJni().ReadMemory();
    }

    public void CloseDeviceClick(View v) {
        /*int nameRet =  d2ccDevice.CloseDevice();
        if(nameRet == 0) Toast.makeText(getApplicationContext(),"Close Success" ,Toast.LENGTH_SHORT).show();
        else Toast.makeText(getApplicationContext(),"Close Error="+nameRet ,Toast.LENGTH_SHORT).show();*/
    }

    public void OpenDeviceClick(View v) {
        D2ccManager.getInstance().OpenDevice();
    }


}
