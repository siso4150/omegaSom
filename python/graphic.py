import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import csv
import glob
import os

# --- 設定 ---
csvDir = "/home/sakai/cppfile/omegaSOM/output"
# 地図の最大サイズ（GISデータの範囲に合わせる）
mapHeight = 79 
mapWidth = 155
weightIdx = 2  # weightRiskを可視化する場合

def loadRoadData(filePath):
    # 背景をNaN（欠損値）で初期化
    grid = np.full((mapHeight, mapWidth), np.nan)
    
    try:
        # numpy.genfromtxtでも読めますが、ヘッダーがある場合はcsvモジュールが確実です
        with open(filePath, 'r') as f:
            reader = csv.reader(f)
            next(reader)  # ヘッダー飛ばし
            for row in reader:
                if not row: continue
                # x, y は整数、weightは浮動小数点
                x, y = int(row[0]), int(row[1])
                val = float(row[weightIdx])
                
                # インデックスが範囲内かチェックして代入
                if 0 <= x < mapWidth and 0 <= y < mapHeight:
                    grid[y, x] = val
    except Exception as e:
        print(f"Error loading {filePath}: {e}")
        return None
    return grid

# ファイルリスト取得
files = sorted(glob.glob(os.path.join(csvDir, "neuron_gen_*.csv")))

fig, ax = plt.subplots(figsize=(8, 4))

# 背景（データがない場所）の色を設定
currentCmap = plt.cm.jet
currentCmap.set_bad(color='lightgray') # NaNの部分を薄いグレーにする

# 初回表示
initialData = loadRoadData(files[0])
im = ax.imshow(initialData, cmap=currentCmap, origin='lower', vmin=0, vmax=2)
plt.colorbar(im, label='Risk Weight')

def update(frame):
    data = loadRoadData(files[frame])
    if data is not None:
        im.set_array(data)
        ax.set_title(f"SOM Progress: {os.path.basename(files[frame])}")
    return [im]

ani = animation.FuncAnimation(fig, update, frames=len(files), interval=100, blit=False)

plt.show()