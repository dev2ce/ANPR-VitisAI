# Triển khai ANPR trên ZCU104 với Vitis AI

## Hướng dẫn cài đặt
- **[Cài đặt Vitis AI Docker image](https://github.com/Xilinx/Vitis-AI/blob/1.3.1/docs/quick-start/install/install_docker/README.md "Install Vitis AI Docker")**
- **[Set up board ZCU104](https://docs.xilinx.com/r/1.3-English/ug1414-vitis-ai/Setting-Up-the-ZCU102/104-Evaluation-Board "ZCU104 setup")**

**Các phiên bản phần mềm được sử dụng trong đồ án:**
- Docker : 20.10.6
- Docker Vitis AI image : 1.3.411   
- Vitis AI : 1.3

Ảnh và video đánh giá tại đường dẫn [link]()

1) Sao chép các thư mục con trong /implementation/ vào thư mục Vitis-AI
2) Sau khi cài đặt Vitis-AI, để khởi động môi trường Vitis AI Docker, tại thư mục Vitis-AI mở terminal chạy lệnh
```
./docker_run.sh xilinx/vitis-ai-cpu:1.3.411
```
3) Kích hoạt môi trường ảo Conda Vitis AI với framework TensorFlow
```
conda activate vitis-ai-tensorflow
```
4) Biên dịch chương trình thực thi
```
cd app
source /opt/petalinux/2020.2/environment-setup-aarch64-xilinx-linux
./build.sh
```
Sao chép chương trình thực thi trong thư mục app đã biên dịch vào thẻ nhớ SD.

5) Cấu hình board và kết nối
Cắm thẻ nhớ SD đã flash image và được sao chép mô hình và chương trình thực thi vào board. Kết nối board với PC thông qua cáp USB đi kèm theo bộ kit, giao thức UART được sử dụng, sử dụng minicom trên Ubuntu:
```
sudo apt update 
sudo apt install minicom -y
sudo minicom -D /dev/ttyUSB1
```
Cấu hình UART như sau:

Bps / Par / Bits: 115200 8N1

Hardware Flow Control: No

6) Chạy chương trình thực thi trên board
Export chuẩn Display Port lên màn hình
```
export DISPLAY=:0.0
xrandr --output DP-1 --mode 1600x900
```

Chạy hệ thống ANPR với video:
```
cd app
./test_video_platerecognition video/footage1.webm
```
Chạy kiểm tra hệ thống ANPR với tập ảnh:
```
./test_accuracy.sh
```


