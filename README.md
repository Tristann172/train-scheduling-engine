# Train Scheduling Engine

## Data structures and complexity

- `LinkedList<T>` is the common manually allocated singly linked container used
  for stations, trains, carriages, and stop schedules. Owning domain
  lists release their entity pointers before clearing their link nodes.
- `HashMap<K, V, HashFunc>` uses separate chaining and resizes its manually
  allocated bucket array. Station IDs and train codes are indexed for expected
  `O(1)` lookup; the built-in hash functions cover `int` and `std::string`.
- `MinHeap<T, Comparator>` is a manually allocated, doubling binary heap. It orders
  schedule events in `O(log E)` per insertion/removal and exposes the next event
  in `O(1)`.
- The scheduler queues station-track and single-track-block occupancy intervals,
  processes them chronologically, and uses a custom open-addressed resource
  table. Schedule changes increment a train revision and enqueue only that
  train's updated events. Older heap entries and resource reservations are
  skipped using lazy revision checks; the event heap is not rebuilt.

The project does not use STL container classes for these structures. Dynamic
arrays and linked nodes are owned and released by their respective classes.

## Lịch nhiều ngày, chỉnh sửa dữ liệu và bảo trì

Thời gian lịch trình là **số phút tuyệt đối tính từ ngày 0**; ví dụ `1500`
là ngày kế tiếp lúc `01:00`. Mô phỏng chạy từ chuyến sớm nhất đến hết lịch
trình, không quay vòng sau 1440 phút. Bảng lịch và đồng hồ hiển thị ngày dạng
`D+1 01:00`.

Có thể ghi lệnh trong `commands.txt`, mỗi lệnh một dòng, các trường phân cách bằng
dấu `|`; dòng bắt đầu bằng `#` là chú thích. Chương trình áp dụng lệnh khi khởi
động và có thể nạp lại từ mục `3.4` trên giao diện:

```text
#ADD_STATION|108|Dong Ha|640|3
#UPDATE_STATION|108|Dong Ha|640|3
#DELETE_STATION|108
#ADD_TRAIN|SE5|1|1
#UPDATE_TRAIN|SE1|1|1
#DELETE_TRAIN|SE5
#ADD_CARRIAGE|SE1|3|GiuongNam
#UPDATE_CARRIAGE|SE1|3|GiuongNam
#DELETE_CARRIAGE|SE1|3
#ADD_STOP|SE1|108|1500|1510|1
#UPDATE_STOP|SE1|108|1500|1520|2
#DELETE_STOP|SE1|108
CLOSE_STATION|104|500|560
CLOSE_BLOCK|104|105|600|660
CLOSE_TRACK|104|2|700|760
OPEN_STATION|104|500|560
```

`CLOSE_STATION` nhận ga và khoảng sửa chữa; `CLOSE_BLOCK` nhận hai ga liền
kề; `CLOSE_TRACK` nhận ga, số ray và khoảng sửa chữa. Dùng lệnh `OPEN_*`
tương ứng với đúng tài nguyên và khoảng thời gian để mở lại. Khoảng bảo trì là
`[bắt đầu, kết thúc)` theo phút tuyệt đối. Khi gặp bảo trì, bộ điều phối sẽ
chuyển ray nếu có ray phù hợp; nếu không, sẽ dời lịch để tàu không sử dụng tài
nguyên đang sửa.

Lệnh thêm/xóa/cập nhật được kiểm tra tính duy nhất, thứ tự ga theo hướng tàu,
thời gian đến/rời và sức chứa ray. Từng dòng bị từ chối được ghi trong kết quả
xử lý `commands.txt`. File lệnh không hỗ trợ thao tác tìm kiếm; tra cứu theo ID
vẫn có trên giao diện. Dòng toa trong `trains.txt` có dạng
`CARRIAGE|số_toa|loại_toa`; trường sức chứa cũ (nếu có) được bỏ qua.

Trên GUI, nhóm `[2]` có thao tác bãi bỏ chuyến, bãi bỏ điểm dừng và điều chỉnh
lịch dừng; các thay đổi lịch đều được kiểm tra điều kiện và chạy lại điều phối.
Nhóm `[3]` đóng/mở ga, ray hoặc khu gian theo đúng ID và khoảng phút đã khai
báo, đồng thời đặt báo cáo trễ giờ. Tìm kiếm ở góc bảng lọc theo ID ga, mã tàu
hoặc ID ga dừng. Mô phỏng tô đỏ tài nguyên đang bảo trì tại thời điểm mô phỏng.