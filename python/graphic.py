import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import copy
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
isPossibleIdx = 3 # ★追加: isPossible列のインデックス（CSVの構造に合わせて変更してください）

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
                isPossible = int(row[isPossibleIdx]) # ★追加: 通行可能フラグの読み込み
                
                if 0 <= x < mapWidth and 0 <= y < mapHeight:
                    if isPossible == 0:
                        grid[y, x] = -1.0  # ★追加: 通行不可の場合は vmin (0) より小さい値を入れる
                    else:
                        grid[y, x] = val
    except Exception as e:
        print(f"Error loading {filePath}: {e}")
        return None
    return grid

# 経路データの読み込み関数
def loadRouteData(filePath):
    xCoords = []
    yCoords = []
    try:
        with open(filePath, 'r') as f:
            reader = csv.reader(f)
            next(reader)
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

# ★修正: matplotlibの仕様変更に対応するため .copy() で独立したカラーマップを生成
currentCmap = copy.copy(plt.cm.jet)
currentCmap.set_bad(color='lightgray')
currentCmap.set_under(color='black')

# 初回表示
initialData = loadRoadData(files[0])
# vmin=0 が設定されているため、-1.0 は set_under の対象になる
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
            routeIdx = gen // 20
            
            # 4桁のゼロ埋めでファイル名を作成 (例: route_0001.csv)
            routeFileName = f"route_{routeIdx:04d}.csv"
            routeFilePath = os.path.join(routeDir, routeFileName)
            
            if os.path.exists(routeFilePath):
                xCoords, yCoords = loadRouteData(routeFilePath)
                if xCoords is not None:
                    routeLine.set_data(xCoords, yCoords)
            else:
                pass # 描画ごとのWarning出力が鬱陶しい場合はここをコメントアウト・消去してください

    return [im, routeLine]

ani = animation.FuncAnimation(fig, update, frames=len(files), interval=100, blit=False)

plt.show()