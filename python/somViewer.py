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

mapHeight = 794
mapWidth = 823
weightIdx = 2
isPossibleIdx = 3 # isPossible列のインデックス

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
                isPossible = int(row[isPossibleIdx]) # 通行可能フラグの読み込み
                
                if 0 <= x < mapWidth and 0 <= y < mapHeight:
                    if isPossible == 0:
                        grid[y, x] = -1.0  # 通行不可の場合は vmin (0) より小さい値を入れる
                    else:
                        grid[y, x] = val
    except Exception as e:
        print(f"Error loading {filePath}: {e}")
        return None
    return grid

# ファイルリスト取得
files = sorted(glob.glob(os.path.join(csvDir, "neuron_gen_*.csv")))

fig, ax = plt.subplots(figsize=(12, 8))

# matplotlibの仕様変更に対応するため .copy() で独立したカラーマップを生成
currentCmap = copy.copy(plt.cm.jet)
currentCmap.set_bad(color='lightgray')
currentCmap.set_under(color='black')

# 初回表示
initialData = loadRoadData(files[0])
# vmin=0 が設定されているため、-1.0 は set_under の対象になる
im = ax.imshow(initialData, cmap=currentCmap, origin='upper', vmin=0, vmax=3)
plt.colorbar(im, label='Risk Weight')

def update(frame):
    filePath = files[frame]
    data = loadRoadData(filePath)
    if data is not None:
        im.set_array(data)
        
    fileName = os.path.basename(filePath)
    currentTime = 0
    match = re.search(r'\d+', fileName)
    if match:
        gen = int(match.group())
        currentTime = gen // 10 + 1  # 20世代ごとに1進む（例: 480世代なら24になる）
        
    # タイトルに現在の時刻（Time）を付与
    ax.set_title(f"SOM Progress: {fileName} (Time: {currentTime})")

    return [im]

ani = animation.FuncAnimation(fig, update, frames=len(files), interval=50, blit=False)

# 保存先のパスとファイル名を指定
outputMoviePath = "som_animation.mp4"

print(f"動画を保存しています... (出力先: {outputMoviePath})")
# MP4として保存（fpsで動画の再生速度を調整）
ani.save(outputMoviePath, writer='ffmpeg', fps=10)
print("動画の保存が完了しました。")

plt.show()