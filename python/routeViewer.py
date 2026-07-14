import numpy as np
import matplotlib.pyplot as plt
import copy
import csv
import os



# --- 設定 ---
csvDir = "/home/sakai/cppfile/som_csv/sample"
routeDir = csvDir + "/route"
snapshotDir = csvDir + "/snapshot_color"
ITER_NUM = 10

os.makedirs(snapshotDir,exist_ok=True)

mapHeight = 483
mapWidth = 884
weightIdx = 2
isPossibleIdx = 3



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

def loadRouteData(filePath):
    dataType = np.dtype([
        ('x','i4'),
        ('y','i4')
    ])
    
    try:
        # バイナリデータを一括読み込み（forループやappendは不要）
        routeData = np.fromfile(filePath, dtype=dataType)
        
        # X座標とY座標の配列（NumPy配列）に分離
        xCoords = routeData['x']
        yCoords = routeData['y']
        
        return xCoords, yCoords
        
    except Exception as e:
        # エラー原因を特定できるようにエラーメッセージを出力することを推奨します
        print(f"Error loading {filePath}: {e}")
        return None, None

def main():
    # カラーマップの準備（ループの外で1回だけ行います）
    currentCmap = copy.copy(plt.cm.jet)
    currentCmap.set_bad(color='lightgray')
    currentCmap.set_under(color='black')

    # 1000から24000まで、1000刻みでループ処理
    for targetGen in range(ITER_NUM, ITER_NUM*24+1, ITER_NUM):
        targetNeuronFile = os.path.join(csvDir, f"neuron_gen_{targetGen:06d}.bin")
        
        # 経路のインデックスを計算 (1000->1, 2000->2 ... 24000->24)
        routeIdx = targetGen // ITER_NUM
        targetRouteFile = os.path.join(routeDir, f"route_{routeIdx:06d}.bin")

        print(f"処理中: 世代 {targetGen} (時間ステップ {routeIdx}) ...")

        roadData = loadRoadData(targetNeuronFile)
        if roadData is None:
            print(f"  -> スキップ: マップデータが見つかりません ({targetNeuronFile})")
            continue

        fig, ax = plt.subplots(figsize=(12, 8))

        im = ax.imshow(roadData, cmap=currentCmap, origin='upper', vmin=0, vmax=3)
        plt.colorbar(im, label='Risk Weight')

        if os.path.exists(targetRouteFile):
            xCoords, yCoords = loadRouteData(targetRouteFile)
            print(f"  ->  経路ファイルを描画 ({targetRouteFile})")
            if xCoords is not None and len(xCoords) > 0:
                ax.plot(xCoords, yCoords, color='purple', linestyle='--', linewidth=2, markersize=3, zorder=10, label='Route')
                
                #スタート地点に印をつける (緑色の大きな丸)
                ax.plot(xCoords[0], yCoords[0], marker='o', color='lime', markersize=10, markeredgecolor='black', zorder=15, label='Start')
                
                #ゴール地点に印をつける (赤色の大きな星)
                ax.plot(xCoords[-1], yCoords[-1], marker='*', color='red', markersize=14, markeredgecolor='black', zorder=15, label='Goal')
                ax.legend(fontsize = "xx-large")
        else:
            print(f"  -> 警告: 経路ファイルが見つかりません ({targetRouteFile})")

        ax.set_title(f"Time Step: {routeIdx}")
        ax.set_xticks([])
        ax.set_yticks([])

        # ゼロ埋めして保存 (例: snapshot_gen_01000.png)
        outputImagePath = os.path.join(snapshotDir, f"snapshot_gen_{targetGen:06d}.png")
        
        # 論文用なのでdpiを高めに設定
        plt.savefig(outputImagePath, dpi=300, bbox_inches='tight')
        
        # ★非常に重要: 連続出力する場合は、メモリあふれを防ぐために必ずfigを閉じる
        plt.close(fig) 

    print("\nすべての画像の出力が完了しました！")

if __name__ == "__main__":
    main()