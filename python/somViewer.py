import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import copy
import csv
import glob
import os
import re

# --- 設定 ---
csvDir = "/home/sakai/cppfile/som_csv/sample"

mapHeight = 483
mapWidth = 884
weightIdx = 2
isPossibleIdx = 3 # isPossible列のインデックス

def loadRoadData(filePath):
    # C++側の構造体とメモリレイアウトを完全に一致させる
    # 'i4': 4バイト整数(int32_t), 'f4': 4バイト浮動小数点数(float)
    dataType = np.dtype([
        ('x', 'i4'),
        ('y', 'i4'),
        ('riskSum', 'f4'),
        ('possible', 'i4')
    ])
    
    # グリッドの初期化（mapHeight, mapWidth はグローバル変数を想定）
    grid = np.full((mapHeight, mapWidth), np.nan)
    
    try:
        # バイナリデータを一括読み込み
        roadData = np.fromfile(filePath, dtype=dataType)
        
        # マップの範囲内に収まっているデータのみを抽出
        validMask = (roadData['x'] >= 0) & (roadData['x'] < mapWidth) & \
                    (roadData['y'] >= 0) & (roadData['y'] < mapHeight)
        
        validData = roadData[validMask]
        
        # 各要素の配列を抽出
        xCoords = validData['x']
        yCoords = validData['y']
        riskSums = validData['riskSum']
        isPossibleFlags = validData['possible']
        
        # isPossible が 0 の場合は -1.0、そうでない場合は riskSum を適用
        finalVals = np.where(isPossibleFlags == 0, -1.0, riskSums)
        
        # グリッドの該当座標へ一括代入
        grid[yCoords, xCoords] = finalVals
        
    except Exception as e:
        print(f"Error loading {filePath}: {e}")
        return None
        
    return grid

# ファイルリスト取得
files = sorted(glob.glob(os.path.join(csvDir, "neuron_gen_*.bin")))

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