package top.frto027.flyric.client;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.NonNull;

import com.maxmpz.poweramp.player.PowerampAPI;
import com.maxmpz.poweramp.player.PowerampAPIHelper;
import com.maxmpz.poweramp.player.RemoteTrackTime;

import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;

import top.frto027.flyric.FlyricClient;
//TODO Time sync
public class WorkService extends Service{
    private static class TitleFormatDictionary{
        private final static String[] Strings = new String[]{
                "PATH","TITLE","ALBUM","ARTIST","CODEC"
        };
        private final static String[] Longs = new String[]{
                "ID","REAL_ID"
        };
        private final static String[] Ints = new String[]{
                "CAT","DURATION","POSITION","POS_IN_LIST","LIST_SIZE","SAMPLE_RATE","CHANNELS","BITRATE","BITS_PER_SAMPLE","FLAGS"
        };
        public static int getType(String name){
            for(String s : Ints)
                if(s.equals(name))
                    return TYPE_INT;
            for(String s : Strings)
                if(s.equals(name))
                    return TYPE_STRING;
            for(String s : Longs)
                if(s.equals(name))
                    return TYPE_LONG;
            return -1;
        }
        public final static int
                TYPE_STRING = 0,
                TYPE_INT = 1,
                TYPE_LONG = 2
                ;
        public String replaceFrom;
        public String nameInBundle;
        public int type;

        public String toString(Bundle currentTrackBundle) {
            switch (type){
                case TYPE_STRING:
                    return currentTrackBundle.getString(nameInBundle,"???");
                case TYPE_INT:
                    return currentTrackBundle.getInt(nameInBundle,0)+"";
                case TYPE_LONG:
                    return currentTrackBundle.getLong(nameInBundle,0)+"";
            }
            return "???";
        }
    }
    public final static String
            ACTION_NAME = "Act",
            ACTION_START = "StartAct",
            ACTION_STOP = "StopAct",
            ACTION_FORMAT_TITLE = "FmtAct",
            ACTION_EXT_OFFSET = "ExtOffsetAct",

            ARG_STR_FORMAT = "fmt",
            ARG_INT_PORT = "port",
            ARG_STR_IP = "ip",
            ARG_SYNC_MODE = "sync_mode",
            ARG_SYNC_TIMEOUT = "sync_timeout",

            ARG_EXT_OFFSET = "ext_offset",

            SYNC_MODE_LOOSE = "sync_loose",
            SYNC_MODE_STRICT = "sync_strict"
            ;

    public final static int DEFAULT_SYNC_TIMEOUT = 5000;

    private int extOffset = 0;

    private final static String TAG = "WorkService";

    /*
    Time in ms. Poweramp's sync api only support 1 sec accurate, but I need more.
    HighFreqSync costs more cpu, but is accurate.
    */
    private final static int minSyncDuration = 5;

    boolean started = false;

    private int config_port = 0;
    private String config_ip = "127.0.0.1";
    private String config_format = "";

    private LinkedList<TitleFormatDictionary> replaceDict;
    private String format_result = "";

    private Intent mTrackIntent = null;
    private Intent mStatusIntent = null;

    private boolean status_playing = false;

    private BroadcastReceiver mPosChangedBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            /* happened many times when sync */
            long time = System.currentTimeMillis();
            //Log.v(TAG,"RECEIVED "+time);
            int pos = intent.getIntExtra(PowerampAPI.Track.POSITION,0);
            updateHighFreqSync(time,pos);
        }
    };

    public WorkService(){
    }

    private void setConfigFormat(String newFormat){
        //reflection add all tags to replaceDict
        replaceDict = new LinkedList<>();
        if(newFormat == null)
            newFormat = "";

        int modifierTag = Modifier.PUBLIC | Modifier.STATIC | Modifier.FINAL;
        Class<PowerampAPI.Track> cls = PowerampAPI.Track.class;
        for(Field f : cls.getDeclaredFields()){
            if((f.getModifiers() & modifierTag) == modifierTag && String.class.isAssignableFrom(f.getType())){
                try {
                    TitleFormatDictionary dItem = new TitleFormatDictionary();
                    dItem.nameInBundle = (String) f.get(null);
                    dItem.replaceFrom = "%" + f.getName().toLowerCase() + "%";
                    dItem.type = TitleFormatDictionary.getType(f.getName());
                    Log.v(TAG,dItem.nameInBundle + " - " + dItem.replaceFrom + " - " + dItem.type);
                    if(dItem.type >= 0 && newFormat.contains(dItem.replaceFrom))
                        replaceDict.add(dItem);
                } catch (IllegalAccessException e) {
                    Log.e(TAG,e.toString());
                }
            }
        }
        config_format = newFormat;
        formatTitle();
    }

    private void requestPosSync(){
        /* setPackage is required here. */
        try {
            startService(new Intent(PowerampAPI.ACTION_API_COMMAND).setPackage(PowerampAPIHelper.getPowerampPackageName(this))
                    .putExtra(PowerampAPI.COMMAND, PowerampAPI.Commands.POS_SYNC));
        }catch (Exception e){
            Log.e(TAG,"",e);
        }
    }


    private Handler handler = new Handler();
    private Runnable highFreqRunnable = new Runnable() {
        @Override
        public void run() {
            requestPosSync();
        }
    };
    private int pos;
    private boolean syncing = false;
    private void highFreqSync(){
        if(syncing)
            return;
        if(status_playing){
            syncing = true;
            pos = -1;
        }
        requestPosSync();
    }

    private void updateHighFreqSync(long timestamp,int pos){
        if(syncing) {
            if(this.pos == -1){
                this.pos = pos;
                handler.post(highFreqRunnable);
            }else if(this.pos == pos){
                /* need more time */
                handler.postDelayed(highFreqRunnable,minSyncDuration - (System.currentTimeMillis() - timestamp));
            }else{
                syncing = false;
                updatePosition(pos,timestamp);
            }
        }else{
            /* temp use */
            updatePosition(pos,timestamp);
            if(status_playing){
                /* I need more! */
                highFreqSync();
            }
        }

    }

    private void startWork(){
        if(started){
            Toast.makeText(this,"work started",Toast.LENGTH_SHORT).show();
            return;
        }

        IntentFilter filter = new IntentFilter(PowerampAPI.ACTION_TRACK_POS_SYNC);
        registerReceiver(mPosChangedBroadcastReceiver, filter);
        highFreqSync();

        started = true;
        /* Debug only */
        /*
        final Handler handler = new Handler();
        handler.post(new Runnable() {
            @Override
            public void run() {
                if(started){
                    handler.postDelayed(this,300);
                }
                requestPosSync();
            }
        });
        /*
         */
    }
    private void stopWork(){
        if(!started){
            Toast.makeText(this,"work is not started",Toast.LENGTH_SHORT).show();
            return;
        }
        started = false;
        unregisterReceiver(mPosChangedBroadcastReceiver);
//        mRemoteTrackTime.unregister();
//        mRemoteTrackTime.setTrackTimeListener(null);
        mTrackIntent = null;
        mStatusIntent = null;
    }
    public void formatTitle(){
        Bundle mCurrentTrack = null;
        if(mTrackIntent != null) {
            mCurrentTrack = mTrackIntent.getBundleExtra(PowerampAPI.TRACK);
        }

        if(mCurrentTrack == null){
            format_result = "";
        }else{
            format_result = config_format;
            for(TitleFormatDictionary dictItem : replaceDict){
                format_result = format_result.replaceAll(dictItem.replaceFrom,dictItem.toString(mCurrentTrack));
            }
        }
    }

    private final FlyricClient client = new FlyricClient();

//=============update position=================

    /**
     * update position
     * @param pos position in sec
     * @param timestamp timestamp in ms
     */
    private void updatePosition(final int pos, final long timestamp){
        Log.v(TAG,"position changed:"+pos);
        if(status_playing){
            new Thread(){
                @Override
                public void run() {
                    synchronized (client){
                        try {
                            client.play_begin(timestamp - pos * 1000 + extOffset);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }.start();
        }else{
            new Thread(){
                @Override
                public void run() {
                    synchronized (client){
                        try {
                            client.pause(pos * 1000);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }.start();
        }
    }
//============update status and track============
    private void updateStatus(){
        int state = mStatusIntent.getIntExtra(PowerampAPI.STATE, PowerampAPI.STATE_NO_STATE);
        int pos = mStatusIntent.getIntExtra(PowerampAPI.Track.POSITION, -1);
        switch (state){
            case PowerampAPI.STATE_PLAYING:
                status_playing = true;
                new Thread(){
                    @Override
                    public void run() {
                        synchronized (client){
                            try {
                                client.play();
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }.start();
                break;
            case PowerampAPI.STATE_PAUSED:
                status_playing = false;
                new Thread(){
                    @Override
                    public void run() {
                        synchronized (client){
                            try {
                                client.pause();
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }.start();
                break;
            default:
            case PowerampAPI.STATE_NO_STATE:
            case PowerampAPI.STATE_STOPPED:
                status_playing = false;
                new Thread(){
                    @Override
                    public void run() {
                        synchronized (client){
                            try {
                                client.pause(0);
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }.start();
                break;
        }


        //TODO send udp package
        Log.d(TAG,"Status:"+mStatusIntent);

        highFreqSync();
    }
    private void updateTrack(){
        if(mTrackIntent != null) {
//            Bundle mCurrentTrack = mTrackIntent.getBundleExtra(PowerampAPI.TRACK);
//            if(mCurrentTrack != null) {
//                int duration = mCurrentTrack.getInt(PowerampAPI.Track.DURATION);
//                mRemoteTrackTime.updateTrackDuration(duration); // Let RemoteTrackTime know about the current song duration.
//            }
//
//            int pos = mTrackIntent.getIntExtra(PowerampAPI.Track.POSITION, -1); // Poweramp build-700+ sends position along with the track intent
//            if(pos != -1) {
//                mRemoteTrackTime.updateTrackPosition(pos);
//            }

            //format
            formatTitle();
            new Thread(){
                @Override
                public void run() {
                    synchronized (client){
                        try {
                            client.load(format_result);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }.start();
            //TODO send udp package
            Log.d(TAG,"Track title:" + format_result);

            highFreqSync();
        }
    }



//===============service=============
    @Override
    public IBinder onBind(Intent intent) {
        return null;/* don't support */
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG,"OnStartCommand");
        if("com.maxmpz.audioplayer.STATUS_CHANGED_EXPLICIT".equals(intent.getAction())){
            if(!started) stopSelf(startId);else{
                mStatusIntent = intent;
                updateStatus();
            }
        }else if("com.maxmpz.audioplayer.TRACK_CHANGED_EXPLICIT".equals(intent.getAction())){
            if(!started) stopSelf(startId);else {
                mTrackIntent = intent;
                updateTrack();
            }
        }else{
            //start or stop service
            String action = intent.getStringExtra(ACTION_NAME);
            if(action == null){
                Log.d(TAG,"Unsupported intent");
            }else if(action.equals(ACTION_START)){
                //TODO init
                config_ip = intent.getStringExtra(ARG_STR_IP);
                if(config_ip == null)
                    config_ip = "127.0.0.1";
                config_format = intent.getStringExtra(ARG_STR_FORMAT);
                if(config_format == null)
                    config_format = "%title%";
                setConfigFormat(config_format);
                config_port = intent.getIntExtra(ARG_INT_PORT,9588);

                extOffset = intent.getIntExtra(ARG_EXT_OFFSET,extOffset);

                try {
                    final InetAddress addr = InetAddress.getByName(config_ip);
                    final int port = config_port;
                    final String sync_mode = intent.getStringExtra(ARG_SYNC_MODE);
                    final int sync_timeout = intent.getIntExtra(ARG_SYNC_TIMEOUT,DEFAULT_SYNC_TIMEOUT);
                    new Thread(){
                        @Override
                        public void run() {
                            synchronized (client){
                                try {
                                    client.connect(addr,port);
                                    if(sync_mode != null){
                                        if(SYNC_MODE_LOOSE.equals(sync_mode)){
                                            client.sync();
                                        }else if(SYNC_MODE_STRICT.equals(sync_mode)){
                                            boolean sync_result = client.sync_strict(sync_timeout);
                                            reportSyncResultAsync(sync_result);
                                        }
                                    }
                                } catch (IOException e) {
                                    Toast.makeText(WorkService.this,"Unknown host.",Toast.LENGTH_SHORT).show();
                                    Log.e(TAG,"",e);
                                }
                            }
                        }
                    }.start();
                } catch (UnknownHostException e) {
                    Toast.makeText(this,"Unknown host.",Toast.LENGTH_SHORT).show();
                }

                startWork();
            }else if(action.equals(ACTION_STOP)){
                //TODO destroy
                stopWork();
                stopSelf(startId);
            }else if(action.equals(ACTION_FORMAT_TITLE)){
                //what should i do? return a formatted title, or send a broadcast?

            }else if(action.equals(ACTION_EXT_OFFSET)){
                extOffset = intent.getIntExtra(ARG_EXT_OFFSET,0);
            }else {
                Log.d(TAG,"Unsupported intent action:"+action);
            }
        }

        return super.onStartCommand(intent, flags, startId);
    }

    private void reportSyncResultAsync(final boolean sync_result) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(WorkService.this,sync_result?R.string.toast_sync_success:R.string.toast_sync_failed,Toast.LENGTH_SHORT).show();
            }
        });
    }
}
