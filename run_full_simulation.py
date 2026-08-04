#!/usr/bin/env python3
import os
import sys
import glob
import subprocess
import time
import pandas as pd
from concurrent.futures import ThreadPoolExecutor, as_completed

def compile_project():
    print("Compiling project with make...")
    res = subprocess.run(["make", "-j16"], cwd="build", capture_output=True, text=True)
    if res.returncode != 0:
        print("Compilation failed!")
        print(res.stderr)
        sys.exit(1)
    print("Compilation successful.")

def clean_csv_files():
    print("Cleaning scan_results_*.csv in build/...")
    csv_files = glob.glob("build/scan_results_*.csv")
    for f in csv_files:
        try:
            os.remove(f)
        except OSError:
            pass

def run_chunk(start, end):
    cmd = ["./crack", "run3.mac", str(start), str(end)]
    log_file = f"build/log_{start}_{end}.txt"
    with open(log_file, "w") as f:
        res = subprocess.run(cmd, cwd="build", stdout=f, stderr=subprocess.STDOUT)
    return start, end, res.returncode

def run_parallel_simulation(target_csv_paths):
    clean_csv_files()
    
    total_points = 10201
    num_workers = 32
    chunk_size = (total_points + num_workers - 1) // num_workers
    
    tasks = []
    for i in range(num_workers):
        start = i * chunk_size
        end = min((i + 1) * chunk_size - 1, total_points - 1)
        if start <= end:
            tasks.append((start, end))
            
    print(f"Starting parallel simulation with {len(tasks)} workers for {total_points} points...")
    
    completed = 0
    start_time = time.time()
    
    with ThreadPoolExecutor(max_workers=num_workers) as executor:
        futures = {executor.submit(run_chunk, start, end): (start, end) for start, end in tasks}
        for future in as_completed(futures):
            start, end = futures[future]
            try:
                start, end, code = future.result()
                completed += 1
                elapsed = time.time() - start_time
                print(f"[{completed}/{len(tasks)}] Finished: {start} -> {end} (code {code}) - Elapsed: {elapsed:.1f}s")
            except Exception as e:
                print(f"Error running chunk {start} -> {end}: {e}")
                
    # Merge CSVs
    print("Merging CSV files...")
    csv_files = glob.glob("build/scan_results_*.csv")
    if not csv_files:
        print("No CSV files found after simulation!")
        return
        
    df_list = [pd.read_csv(f) for f in csv_files]
    merged_df = pd.concat(df_list, ignore_index=True)
    merged_df.sort_values(by=['x', 'y', 'detectorID'], inplace=True)
    
    for path in target_csv_paths:
        os.makedirs(os.path.dirname(path) if os.path.dirname(path) else '.', exist_ok=True)
        merged_df.to_csv(path, index=False)
        print(f"Saved merged CSV to: {path}")

def set_crack_state(enable):
    path = "src/DetectorConstruction.cc"
    with open(path, "r") as f:
        lines = f.readlines()
    
    start_idx = None
    end_idx = None
    for idx, line in enumerate(lines):
        if "auto solidVoid =" in line:
            start_idx = idx
        if "logicVoid->SetVisAttributes(" in line:
            for j in range(idx, len(lines)):
                if ");" in lines[j]:
                    end_idx = j
                    break
            break
            
    if start_idx is None or end_idx is None:
        print("Error: Could not find VoidSolid block boundaries in DetectorConstruction.cc")
        sys.exit(1)
        
    is_commented = False
    if start_idx > 0 and "/*" in lines[start_idx - 1]:
        is_commented = True
        
    if enable:
        if is_commented:
            print("Enabling crack (uncommenting VoidSolid block)...")
            if "/*" in lines[start_idx - 1] and "*/" in lines[end_idx + 1]:
                del lines[end_idx + 1]
                del lines[start_idx - 1]
            else:
                print("Warning: Comment tags not in expected positions, doing nothing.")
        else:
            print("Crack is already enabled.")
    else:
        if not is_commented:
            print("Disabling crack (commenting out VoidSolid block)...")
            lines.insert(end_idx + 1, "    */\n")
            lines.insert(start_idx, "    /*\n")
        else:
            print("Crack is already disabled.")
            
    with open(path, "w") as f:
        f.writelines(lines)

def main():
    # 1. Run simulation for CRACK
    print("=== CRACK SIMULATION ===")
    set_crack_state(True)
    compile_project()
    run_parallel_simulation(["new_crack.csv", "new_crack/new_crack.csv"])
    
    # 2. Run simulation for NORMAL
    print("=== NORMAL SIMULATION ===")
    set_crack_state(False)
    compile_project()
    run_parallel_simulation(["normal/new_normal.csv", "new_normal.csv"])
    
    # 3. Restore to CRACK
    print("=== RESTORING CRACK STATE ===")
    set_crack_state(True)
    compile_project()
    
    # 4. Run post-processing
    print("=== CALCULATING RELATIVE DIFFERENCES ===")
    subprocess.run(["python3", "process_relative_difference.py", "normal/new_normal.csv", "new_crack.csv"])
    print("All simulations and post-processing completed successfully!")

if __name__ == '__main__':
    main()
