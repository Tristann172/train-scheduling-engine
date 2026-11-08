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
động và có thể nạp lại bằng nút "Áp dụng commands.txt" trên màn Nhà ga:

```text
#ADD_STATION|119|Thap Cham|1408|3
#UPDATE_STATION|119|Thap Cham|1408|3
#DELETE_STATION|119
#ADD_TRAIN|SE5|1|1
#UPDATE_TRAIN|SE1|1|1
#DELETE_TRAIN|SE5
#ADD_CARRIAGE|SE1|3|GiuongNam
#UPDATE_CARRIAGE|SE1|3|GiuongNam
#DELETE_CARRIAGE|SE1|3
#ADD_STOP|SE1|119|1500|1510|1
#UPDATE_STOP|SE1|119|1500|1520|2
#DELETE_STOP|SE1|119
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

## Giao diện

Thanh trên cùng có các màn sau, cửa sổ có thể phóng to tùy ý mà chữ vẫn sắc nét:

- **Nhà ga**: bản đồ Việt Nam rút gọn, bảng ga có tìm theo ID, thêm ga, quét xung
  đột, áp dụng `commands.txt` và nạp lại từ file.
- **Tàu & lịch trình**: danh sách đoàn tàu, toa xe và bảng lịch dừng. Bấm một dòng
  lịch để điều chỉnh giờ, đổi ray hoặc bãi bỏ điểm dừng trong cùng một biểu mẫu.
- **Bảo trì & sự cố**: danh sách lịch bảo trì, đóng/mở ga, khu gian, ray và báo tàu
  trễ. Thao tác đóng/mở được ghi thêm vào `commands.txt`.
- **Bản đồ mô phỏng**: tàu chạy trên bản đồ theo thời gian, xem toàn quốc hoặc từng
  miền, đoạn bảo trì có sọc đỏ vàng. Phím Space để chạy/dừng, mũi tên trái/phải để
  tua 15 phút.
- **Đồ thị**: tiến trình từng tàu so với lịch công bố và lý do tàu phải chờ.

Giờ nhập theo dạng `09:20` hoặc `D+1 01:00`.

## Dữ liệu

- `stations.txt`, `trains.txt`, `commands.txt`: dữ liệu vận hành, chương trình tự
  ghi lại `trains.txt` sau khi điều phối.
- `timetable_base.txt`: lịch công bố để so sánh độ trễ, chương trình chỉ đọc. Giờ
  SE1 đến SE4 lấy từ công cụ tra cứu của Đường sắt Việt Nam ngày 08/11/2026, ngày 0
  là 10/11/2026.
- Số ray trong `stations.txt` là số ước lượng từ OpenStreetMap, không phải số liệu
  chính thức. Hai lịch bảo trì trong `commands.txt` là ví dụ minh họa.
- `map_data.txt`: đường bờ biển và đường ray lấy từ Natural Earth, tọa độ ga đối
  chiếu với OpenStreetMap. Ga mới được đặt lên tuyến theo lý trình.
- `assets/fonts`: font Barlow (giấy phép SIL OFL, xem `OFL.txt`).

## Biên dịch

Cần SFML 3.1 và C++17. Với MinGW:

```text
g++ -std=c++17 main.cpp FileManager.cpp GraphicalSimulationEngine.cpp GuiMenu.cpp MapData.cpp StationList.cpp TrainList.cpp UiKit.cpp -IC:/SFML-3.1.0/include -LC:/SFML-3.1.0/lib -lsfml-graphics -lsfml-window -lsfml-system -o TrainScheduling.exe
```

Với MSVC cần thêm cờ `/utf-8`. Chạy chương trình tại thư mục chứa các file dữ liệu
và thư mục `assets`.
