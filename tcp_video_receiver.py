import socket
import struct
import time

import cv2
import numpy as np

# --- 配置 ---
TCP_IP = "0.0.0.0"
TCP_PORT = 8080
HEADER_FMT = "<H I H H H Q"
HEADER_SIZE = struct.calcsize(HEADER_FMT)
MAGIC_NUMBER = 0xABCD


def recv_exact(sock, size):
    """从 TCP 流中精确读取 size 字节，连接关闭时返回 None。"""
    chunks = bytearray()
    while len(chunks) < size:
        packet = sock.recv(size - len(chunks))
        if not packet:
            return None
        chunks.extend(packet)
    return bytes(chunks)


def draw_fps(img, fps):
    fps_text = f"FPS: {int(fps)}"
    h, w = img.shape[:2]
    cv2.putText(
        img,
        fps_text,
        (w - 120, h - 20),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (0, 255, 0),
        2,
    )


def handle_client(client_sock, client_addr):
    frame_buffer = {}
    max_f_id = -1
    prev_time = 0.0
    fps = 0.0

    print(f"Client connected: {client_addr[0]}:{client_addr[1]}")

    while True:
        header_data = recv_exact(client_sock, HEADER_SIZE)
        if header_data is None:
            print("Client disconnected")
            break

        magic, f_id, p_cnt, p_id, d_len, ts = struct.unpack(HEADER_FMT, header_data)
        if magic != MAGIC_NUMBER:
            print(f"Invalid magic: 0x{magic:04X}")
            continue

        payload = recv_exact(client_sock, d_len)
        if payload is None:
            print("Client disconnected while receiving payload")
            break

        if f_id < max_f_id:
            continue

        if f_id > max_f_id:
            max_f_id = f_id
            frame_buffer.clear()
            frame_buffer[f_id] = [None] * p_cnt

        if p_id >= p_cnt:
            continue

        frame_buffer[f_id][p_id] = payload

        if max_f_id in frame_buffer and all(p is not None for p in frame_buffer[max_f_id]):
            full_jpg = b"".join(frame_buffer[max_f_id])
            img = cv2.imdecode(np.frombuffer(full_jpg, np.uint8), cv2.IMREAD_COLOR)

            if img is not None:
                curr_time = time.time()
                time_diff = curr_time - prev_time
                if time_diff > 0:
                    real_fps = 1.0 / time_diff
                    fps = (fps * 0.9) + (real_fps * 0.1)
                prev_time = curr_time

                draw_fps(img, fps)
                cv2.imshow("TCP Stream", img)

            frame_buffer.clear()

            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

    client_sock.close()
    cv2.destroyAllWindows()


def start_receiver():
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind((TCP_IP, TCP_PORT))
    server_sock.listen(1)

    print(f"Listening on {TCP_IP}:{TCP_PORT}...")

    try:
        while True:
            client_sock, client_addr = server_sock.accept()
            handle_client(client_sock, client_addr)
    except KeyboardInterrupt:
        print("\nReceiver stopped")
    finally:
        server_sock.close()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    start_receiver()
