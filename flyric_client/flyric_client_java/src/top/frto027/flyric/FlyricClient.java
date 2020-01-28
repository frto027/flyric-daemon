package top.frto027.flyric;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.Date;
import java.util.Scanner;
import java.util.concurrent.atomic.AtomicInteger;

public class FlyricClient {
    private final static int
            UDP_DATA_TYPE_LOAD_LYRIC     = 1,
            UDP_DATA_TYPE_PLAY_TIME     = 2,
            UDP_DATA_TYPE_PAUSE_TIME    = 3,
            UDP_DATA_TYPE_PLAY_NOW      = 4,
            UDP_DATA_TYPE_PAUSE_NOW     = 5,
            UDP_DATA_TYPE_SYNC_NOW      = 6;

    private DatagramSocket socket = null;

    private DatagramPacket play_packet,pause_packet,play_now_packet,pause_now_packet,sync_packet;
    private ByteBuffer play_packet_buffer,pause_packet_buffer,play_now_packet_buffer,pause_now_packet_buffer,sync_packet_buffer;

    private AtomicInteger counter = new AtomicInteger(-1);

    private InetAddress target_addr;
    private int target_port;

    private long timeOffset = 0;
    private long getTime(){
        return getRealTime() + timeOffset;
    }
    private long getRealTime(){
        return new Date().getTime();
    }

    public void connect(InetAddress addr,int port) throws SocketException {
        if(socket != null){
            socket.close();
        }
        socket = new DatagramSocket();
        target_addr = addr;
        target_port = port;

        byte[] bts;
        play_packet_buffer = ByteBuffer.allocate(4+4+8).order(ByteOrder.BIG_ENDIAN);
        bts = play_packet_buffer
                .putInt(0)//id
                .putInt(UDP_DATA_TYPE_PLAY_TIME)
                .putLong(0)
                .array();
        play_packet = new DatagramPacket(bts,bts.length,target_addr,target_port);

        pause_packet_buffer = ByteBuffer.allocate(4+4+8).order(ByteOrder.BIG_ENDIAN);
        bts = pause_packet_buffer
                .putInt(0)//id
                .putInt(UDP_DATA_TYPE_PAUSE_TIME)
                .putLong(0)
                .array();
        pause_packet = new DatagramPacket(bts,bts.length,target_addr,target_port);

        play_now_packet_buffer = ByteBuffer.allocate(4+4).order(ByteOrder.BIG_ENDIAN);
        bts = play_now_packet_buffer
                .putInt(0)//id
                .putInt(UDP_DATA_TYPE_PLAY_NOW)
                .array();
        play_now_packet = new DatagramPacket(bts,bts.length,target_addr,target_port);

        pause_now_packet_buffer = ByteBuffer.allocate(4+4).order(ByteOrder.BIG_ENDIAN);
        bts = pause_now_packet_buffer
                .putInt(0)//id
                .putInt(UDP_DATA_TYPE_PAUSE_NOW)
                .array();
        pause_now_packet = new DatagramPacket(bts,bts.length,target_addr,target_port);

        sync_packet_buffer = ByteBuffer.allocate(4+4+8).order(ByteOrder.BIG_ENDIAN);
        bts = sync_packet_buffer
                .putInt(0)//id
                .putInt(UDP_DATA_TYPE_SYNC_NOW)
                .putLong(0)
                .array();
        sync_packet = new DatagramPacket(bts,bts.length,target_addr,target_port);
    }

    public void load(String name) throws IOException {
        if(socket == null)
            return;
        try {
            byte[] bts = name.getBytes("utf-8");
            bts = ByteBuffer.allocate(4 + 4 + bts.length + 1)
                    .order(ByteOrder.BIG_ENDIAN)
                    .putInt(0)//init id
                    .putInt(UDP_DATA_TYPE_LOAD_LYRIC)
                    .put(bts)
                    .put((byte)0)
                    .array();
            counter.set(0);
            DatagramPacket packet = new DatagramPacket(bts,bts.length,target_addr,target_port);
            socket.send(packet);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
    }

    public void pause(long time_ms) throws IOException {
        if(socket == null)
            return;
        pause_packet_buffer.position(0);
        pause_packet_buffer.putInt(counter.incrementAndGet());
        pause_packet_buffer.position(8);
        pause_packet_buffer.putLong(time_ms);
        socket.send(pause_packet);
    }

    public void pause() throws IOException {
        if(socket == null)
            return;
        pause_now_packet_buffer.position(0);
        pause_now_packet_buffer.putInt(counter.incrementAndGet());
        socket.send(pause_now_packet);
    }
    //使用系统时钟同步
    public void play(long time_ms) throws IOException{
        play_begin(getTime() - time_ms);
    }
    //发送原始数据
    public void play_begin(long begin_time_ms) throws IOException {
        if(socket == null)
            return;
        play_packet_buffer.position(0);
        play_packet_buffer.putInt(counter.incrementAndGet());
        play_packet_buffer.position(8);
        play_packet_buffer.putLong(begin_time_ms);
        socket.send(play_packet);
    }

    public void play() throws IOException {
        if(socket == null)
            return;
        play_now_packet_buffer.position(0);
        play_now_packet_buffer.putInt(counter.incrementAndGet());
        socket.send(play_now_packet);
    }

    public void sync() throws IOException {
        sync(false,0);
    }
    public boolean sync_strict(int timeout) throws IOException {
        return sync(true,timeout);
    }

    private boolean sync(boolean strict,int timeout) throws IOException {
        if(socket == null)
            return false;
        sync_packet_buffer.position(0);
        sync_packet_buffer.putInt(counter.incrementAndGet());
        sync_packet_buffer.position(8);
        sync_packet_buffer.putLong(getRealTime());
        socket.send(sync_packet);
        if(strict){
            int origin = socket.getSoTimeout();
            socket.setSoTimeout(timeout);
            try{
                socket.receive(sync_packet);
                long time2 = getRealTime();
                sync_packet_buffer.position(8);
                long time1 = sync_packet_buffer.getLong();
                timeOffset = (time1 + (time2 - time1)/2) - time2;
                return true;
            }catch (SocketTimeoutException e){
                //do nothing
                return false;
            }finally {
                socket.setSoTimeout(origin);
            }
        }else{
            return false;
        }
    }
	//Debug only
    public static void main(String [] a) throws IOException {
        /*

        Input commands in console.For example:
        conn 9588
        pause
        play
        pause 6000
        play 5000
        play 0

        sync
        syncs 1000
        load lyric_path

         */
        Scanner scanner = new Scanner(System.in);
        String s;
        FlyricClient client = new FlyricClient();
        while((s = scanner.nextLine()) != null){
            String[] chs = s.split(" ");
            if(chs.length == 0){
                continue;
            }
            if(chs[0].equals("conn")){
                client.connect(InetAddress.getByName("localhost"),Integer.parseInt(chs[1]));
            }
            if(chs[0].equals("play")){
                if(chs.length > 1){
                    client.play(Integer.parseInt(chs[1]));
                }else{
                    client.play();
                }
            }
            if(chs[0].equals("pause")){
                if(chs.length > 1){
                    client.pause(Integer.parseInt(chs[1]));
                }else{
                    client.pause();
                }
            }
            if(chs[0].equals("load")){
                String ld = chs.length > 1 ? chs[1] : "";
                client.load(ld);
            }
            if(chs[0].equals("sync")){
                client.sync();
            }
            if(chs[0].equals("syncs")){
                client.sync_strict(Integer.parseInt(chs[1]));
                System.out.println("[sync over]");
            }
        }
    }
}
