import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import csv
import glob
import os
import re

# --- 設定 ---
csvDir = "/home/sakai/cppfile/omegaSOM/output"
routeDir = "/home/sakai/cppfile/omegaSOM/output/route" # 経路CSVのディレクトリ

mapHeight = 79 
mapWidth = 155
weightIdx = 2

def loadRoadData(filePath):
    grid = np.full((mapHeight, mapWidth), np.nan)
    try:
        with open(filePath, 'r') as f:
            reader = csv.reader(f)
            next(reader)  # ヘッダー飛ばし
            for row in reader:
                if not row: continue
                x, y = int(row[0]), int(row[1])
                val = float(row[weightIdx])
                
                if 0 <= x < mapWidth and 0 <= y < mapHeight:
                    grid[y, x] = val
    except Exception as e:
        print(f"Error loading {filePath}: {e}")
        return None
    return grid

# 経路データの読み込み関数
# 経路データの読み込み関数
def loadRouteData(filePath):
    xCoords = []
    yCoords = []
    try:
        with open(filePath, 'r') as f:
            reader = csv.reader(f)
            next(reader)  # ここを有効化してヘッダーを読み飛ばす
            for row in reader:
                if not row: continue
                # 0列目をX座標、1列目をY座標として取得
                xCoords.append(float(row[0]))
                yCoords.append(float(row[1]))
    except Exception as e:
        print(f"Error loading route {filePath}: {e}")
        return None, None
    return xCoords, yCoords

# ファイルリスト取得
files = sorted(glob.glob(os.path.join(csvDir, "neuron_gen_*.csv")))

fig, ax = plt.subplots(figsize=(8, 4))
currentCmap = plt.cm.jet
currentCmap.set_bad(color='lightgray')

# 初回表示
initialData = loadRoadData(files[0])
im = ax.imshow(initialData, cmap=currentCmap, origin='upper', vmin=0, vmax=2)
plt.colorbar(im, label='Risk Weight')

# 経路描画用のラインオブジェクトを初期化（最初は空）
routeLine, = ax.plot([], [], color='red', linewidth=2, marker='o', markersize=3, zorder=10)

def update(frame):
    filePath = files[frame]
    data = loadRoadData(filePath)
    if data is not None:
        im.set_array(data)
        
    fileName = os.path.basename(filePath)
    ax.set_title(f"SOM Progress: {fileName}")
    
    # 正規表現でファイル名から世代の数値を取り出す
    match = re.search(r'\d+', fileName)
    if match:
        gen = int(match.group())
        
        # 20の倍数の世代のときだけ経路を更新する
        if gen > 0 and gen % 20 == 0:
            # gen=20 -> 1, gen=40 -> 2 となるようにインデックスを計算
            routeIdx = gen // 20
            
            # 4桁のゼロ埋めでファイル名を作成 (例: route_0001.csv)
            routeFileName = f"route_{routeIdx:04d}.csv"
            routeFilePath = os.path.join(routeDir, routeFileName)
            
            if os.path.exists(routeFilePath):
                xCoords, yCoords = loadRouteData(routeFilePath)
                if xCoords is not None:
                    # 経路を更新
                    routeLine.set_data(xCoords, yCoords)
            else:
                print(f"Warning: Route file not found -> {routeFileName}")

    return [im, routeLine]

ani = animation.FuncAnimation(fig, update, frames=len(files), interval=100, blit=False)

plt.show()