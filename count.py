from pathlib import Path

if __name__ == "__main__":
    total_lines = 0
    root = Path(".")
    
    # Find all .c and .h files recursively, excluding anything in a 'build' directory
    files = [
        f for f in list(root.rglob('*.c')) + list(root.rglob('*.h'))
        if 'build' not in f.parts
    ]
    
    for file_path in files:
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                file_count = sum(1 for _ in f)
                print(f"{file_path}: {file_count}")
                total_lines += file_count
        except Exception as e:
            print(f"Could not read {file_path}: {e}")
            
    print(f"\nTotal lines: {total_lines}")
