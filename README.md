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

Toàn bộ thao tác thêm/sửa/xóa được thực hiện trên giao diện SFML. Các ràng buộc
về tính duy nhất, thứ tự ga theo hướng tàu, thời gian đến/rời và sức chứa ray
được kiểm tra trước khi ghi. Khoảng bảo trì là `[bắt đầu, kết thúc)` theo phút
tuyệt đối; khi gặp bảo trì, bộ điều phối sẽ chuyển ray nếu có ray phù hợp,
nếu không sẽ dời lịch để tàu không sử dụng tài nguyên đang sửa.

Các lịch bảo trì được lưu trực tiếp trong `trains.txt` theo dạng
`MAINTENANCE|STATION|ga|0|bắt_đầu|kết_thúc`,
`MAINTENANCE|BLOCK|ga1|ga2|bắt_đầu|kết_thúc` hoặc
`MAINTENANCE|TRACK|ga|ray|bắt_đầu|kết_thúc`. Dòng toa trong `trains.txt` có dạng
`CARRIAGE|số_toa|loại_toa`; trường sức chứa cũ (nếu có) được bỏ qua.

## Giao diện

Thanh trên cùng có các màn sau, cửa sổ có thể phóng to tùy ý mà chữ vẫn sắc nét:

- **Nhà ga**: bản đồ Việt Nam rút gọn, bảng ga có tìm theo ID, thêm/sửa/xóa ga,
  quét xung đột và nạp lại từ file.
- **Tàu & lịch trình**: danh sách đoàn tàu, loại tàu đa hình, toa xe và bảng lịch
  dừng. Có thể thêm/sửa/xóa tàu, toa xe và điểm dừng trực tiếp trên giao diện.
- **Bảo trì & sự cố**: danh sách lịch bảo trì, đóng/mở ga, khu gian, ray và báo tàu
  trễ. Thao tác đóng/mở được lưu trực tiếp vào `trains.txt`.
- **Bản đồ mô phỏng**: tàu chạy trên bản đồ theo thời gian, xem toàn quốc hoặc từng
  miền, đoạn bảo trì có sọc đỏ vàng. Phím Space để chạy/dừng, mũi tên trái/phải để
  tua 15 phút.
- **Đồ thị**: tiến trình từng tàu so với lịch công bố và lý do tàu phải chờ.

Giờ nhập theo dạng `09:20` hoặc `D+1 01:00`.

## Dữ liệu

- `data/stations.txt`, `data/trains.txt`: dữ liệu vận hành, chương trình tự ghi
  lại sau khi điều phối hoặc CRUD trên giao diện.
- `data/timetable_base.txt`: lịch công bố để so sánh độ trễ, chương trình chỉ đọc. Giờ
  SE1 đến SE4 lấy từ công cụ tra cứu của Đường sắt Việt Nam ngày 08/11/2026, ngày 0
  là 10/11/2026.
- Số ray trong `data/stations.txt` là số ước lượng từ OpenStreetMap, không phải số liệu
  chính thức. Hai lịch bảo trì mẫu được lưu trong `data/trains.txt`.
- `data/map_data.txt`: đường bờ biển và đường ray lấy từ Natural Earth, tọa độ ga đối
  chiếu với OpenStreetMap. Ga mới được đặt lên tuyến theo lý trình.
- `assets/fonts`: font Barlow và Arial (giấy phép SIL OFL của Barlow xem `OFL.txt`).

## Biên dịch

Cần SFML 3.1 và C++17. Cấu trúc thư mục chính gồm `src/` (mã nguồn), `include/`
(header), `data/` (dữ liệu), `assets/` (font), `tools/` (công cụ) và `bin/` (DLL).
Với MinGW, có thể biên dịch trực tiếp:

```text
g++ -std=c++17 src/main.cpp src/FileManager.cpp src/GraphicalSimulationEngine.cpp src/GuiMenu.cpp src/MapData.cpp src/StationList.cpp src/TrainList.cpp src/UiKit.cpp -Iinclude -IC:/SFML-3.1.0/include -LC:/SFML-3.1.0/lib -lsfml-graphics -lsfml-window -lsfml-system -o bin/TrainScheduling.exe
```

Hoặc dùng CMake 3.21+:

```text
cmake -S . -B build -DSFML_DIR=C:/SFML-3.1.0/lib/cmake/SFML
cmake --build build
```

Chạy từ thư mục gốc của dự án để các đường dẫn dữ liệu và font được phân giải đúng.
Với lệnh MinGW, chạy `.\bin\TrainScheduling.exe`; với CMake chạy
`.\build\bin\TrainScheduling.exe`. CMake tự sao chép SFML DLL cạnh file chạy.
Có thể đặt `SFML_DIR` trỏ tới thư mục cấu hình SFML của máy. Với MSVC cần bật `/utf-8`.
