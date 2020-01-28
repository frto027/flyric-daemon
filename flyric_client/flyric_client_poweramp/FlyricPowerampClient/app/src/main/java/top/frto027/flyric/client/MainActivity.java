package top.frto027.flyric.client;

import androidx.appcompat.app.AppCompatActivity;

import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;

import com.maxmpz.poweramp.player.PowerampAPI;
import com.maxmpz.poweramp.player.RemoteTrackTime;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    private final static String TAG = "FlyricPowerampClient";
    private EditText ipText,portText,formatText;
    private CheckBox syncCb;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        findViewById(R.id.startBtn).setOnClickListener(this);
        findViewById(R.id.stopBtn).setOnClickListener(this);

        ipText = findViewById(R.id.ipText);
        portText = findViewById(R.id.portText);
        formatText = findViewById(R.id.formatText);
        syncCb = findViewById(R.id.syncCb);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.startBtn: {
                int port = 9588;
                try {
                    port = Integer.parseInt(portText.getText().toString());
                } catch (NumberFormatException e) {
                    Log.e(TAG, "", e);
                }
                Intent intent = new Intent(this, WorkService.class)
                        .putExtra(WorkService.ACTION_NAME, WorkService.ACTION_START)
                        .putExtra(WorkService.ARG_STR_IP, ipText.getText().toString())
                        .putExtra(WorkService.ARG_INT_PORT, port)
                        .putExtra(WorkService.ARG_STR_FORMAT, formatText.getText().toString());
                if(syncCb.isChecked()){
                    intent = intent
                            .putExtra(WorkService.ARG_SYNC_MODE,WorkService.SYNC_MODE_STRICT);
                }
                ComponentName name = startService(
                        intent
                );
                Log.d(TAG, "Start:" + name);
            }
            break;
            case R.id.stopBtn: {
                ComponentName name = startService(
                        new Intent(this, WorkService.class).
                                putExtra(WorkService.ACTION_NAME, WorkService.ACTION_STOP)
                );
                Log.d(TAG, "Stop:" + name);
            }
            break;
        }
    }
}
