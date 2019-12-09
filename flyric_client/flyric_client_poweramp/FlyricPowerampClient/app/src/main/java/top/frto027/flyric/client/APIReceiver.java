package top.frto027.flyric.client;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class APIReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        context.startService(
               intent.setClass(context,WorkService.class)
        );
    }
}
