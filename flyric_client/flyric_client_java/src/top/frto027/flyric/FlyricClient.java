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
            UDP_DATA_TYPE_LOADLYRIC     = 1,
            UDP_DATA_TYPE_PLAY_TIME     = 2,
            UDP_DATA_TYPE_PAUSE_TIME    = 3,
            UDP_DATA_TYPE_PLAY_NOW      = 4,
            UDP_DATA_TYPE_PAUSE_NOW     = 5;

    DatagramSocket socket = null;

    DatagramPacket play_packet,pause_packet,play_now_packet,pause_now_packet;
    ByteBuffer play_packet_buffer,pause_packet_buffer,play_now_packet_buffer,pause_now_packet_buffer;

    private AtomicInteger counter = new AtomicInteger(0);

    InetAddress target_addr;
    int target_port;
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

    }

    public void load(String name) throws IOException {
        if(socket == null)
            return;
        try {
            byte[] bts = name.getBytes("utf-8");
            bts = ByteBuffer.allocate(4 + 4 + bts.length + 1)
                    .order(ByteOrder.BIG_ENDIAN)
                    .putInt(0)//init id
                    .putInt(UDP_DATA_TYPE_LOADLYRIC)
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

    public void pause(long timems) throws IOException {
        if(socket == null)
            return;
        pause_packet_buffer.position(0);
        pause_packet_buffer.putInt(counter.incrementAndGet());
        pause_packet_buffer.position(8);
        pause_packet_buffer.putLong(timems);
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
    public void play(long timems) throws IOException{
        play_begin(new Date().getTime() - timems);
    }
    //发送原始数据
    public void play_begin(long begin_timems) throws IOException {
        if(socket == null)
            return;
        play_packet_buffer.position(0);
        play_packet_buffer.putInt(counter.incrementAndGet());
        play_packet_buffer.position(8);
        play_packet_buffer.putLong(begin_timems);
        socket.send(play_packet);
    }

    public void play() throws IOException {
        if(socket == null)
            return;
        play_now_packet_buffer.position(0);
        play_now_packet_buffer.putInt(counter.incrementAndGet());
        socket.send(play_now_packet);
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
        }
    }
}
