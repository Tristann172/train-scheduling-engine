import os

def merge_cpp_project(output_file="merged_code.txt"):
    # Chỉ định các đuôi file C++ cần lấy
    valid_exts = {'.cpp', '.h', '.hpp'}
    
    with open(output_file, 'w', encoding='utf-8') as outfile:
        for root, dirs, files in os.walk('.'):
            # Bỏ qua các thư mục ẩn hoặc thư mục build nếu có
            dirs[:] = [d for d in dirs if d not in {'.git', '.vscode', 'build', 'out'}]
            
            for file in files:
                ext = os.path.splitext(file)[1]
                # Chỉ xử lý nếu đúng định dạng C++
                if ext in valid_exts:
                    filepath = os.path.join(root, file)
                    try:
                        with open(filepath, 'r', encoding='utf-8') as infile:
                            outfile.write(f"\n{'='*40}\n")
                            outfile.write(f"File: {filepath}\n")
                            outfile.write(f"{'='*40}\n\n")
                            outfile.write(infile.read())
                            outfile.write("\n")
                    except Exception as e:
                        print(f"Không thể đọc {filepath}: {e}")

if __name__ == "__main__":
    merge_cpp_project()
    print("Đã gộp xong các file .cpp và .h thành merged_code.txt!")