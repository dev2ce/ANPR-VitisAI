## Huấn luyện
Sử dụng framework Darknet để huấn luyện và đánh giá mô hình trên GPU. Chi tiết về cách cài đặt và huấn luyện có thể tham khảo tại [đây](https://github.com/AlexeyAB/darknet#how-to-compile-on-linux-using-make "Darknet").
Tập dữ liệu được sử dụng để huấn luyện [link](https://drive.google.com/file/d/1WJfI7YKfMf6gWIZzzWb7Du8mJifEnDyX/view?usp=drive_link)

1. Sau khi cài đặt darknet, sao chép các folder con của `CR-dataset`, `VD-dataset`, `PD-dataset` tập dữ liệu vào trong đường dẫn darknet/data. 
2. Lưu ý là tất cả file `.cfg`, `.data`, `.names`, `train.txt` và `valid.txt` phải để trong đường dẫn darknet/data.
