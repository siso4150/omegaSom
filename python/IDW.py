import pandas as pd
import numpy as np
from pyproj import Transformer
from scipy.spatial.distance import cdist


#緯度経度→xy座標の変換
coordTransformer = Transformer.from_crs(
    "EPSG:4326","EPSG:6676",always_xy=True
)

#気象台の座標
observatory = {
    'hamamatsu':{'lon': 137.427, 'lat': 34.452},
    'iwata' : {'lon': 137.528, 'lat': 34.415},
    'tenryu': {'lon': 137.488, 'lat': 34.534}
}

meterObservatoryData = {}
for stationName, coordinates in observatory.items():
    # 辞書から経度(lon)と緯度(lat)を取得して変換
    meterX, meterY = coordTransformer.transform(
        coordinates["lon"], coordinates["lat"]
    )
    # メートル単位の座標を新しい辞書に格納
    meterObservatoryData[stationName] = {"meterX": meterX, "meterY": meterY}
    
observatoryDf = pd.DataFrame(meterObservatoryData).T

#メッシュデータの読み込み
mesh = pd.read_csv("hamamatsu_mesh.csv")
#メッシュの緯度経度をxyに変換して、追加
meterX,meterY = coordTransformer.transform(mesh['lon'].values,mesh['lat'].values)
mesh["meterX"] = meterX
mesh["meterY"] = meterY

#観測所の座標情報(x,y)だけ取り出す
obCoord = observatoryDf[["meterX","meterY"]].values
#メッシュも
meshCoord = mesh[["meterX","meterY"]].values

#距離行列(di)を計算（推定点数,観測所数)の形
distanceMatrix = cdist(meshCoord,obCoord,"euclidean")

#重み行列(wi)を計算
p = 1.0 #べき乗値
weightMatrix = 1.0 / (distanceMatrix**p)


normalizedWeights = weightMatrix / np.sum(weightMatrix, axis=1, keepdims=True)

#天候データを読み込み
precipitationDf = pd.read_csv("rain.csv")  # 降水量
windDf = pd.read_csv("windSpeed.csv")  # 風速
temperatureDf = pd.read_csv("temperature.csv")  # 気温


# #静的データを出力　既存のものから、必要なものだけ
staticColumns = ["bufferRadius_max","col_index","row_index","dem_mean"]
mesh_base = mesh[staticColumns]
mesh_base.to_csv("mesh_base.csv", index=False)
print("mesh_base.csv を出力しました。")

totalTimeSteps = len(windDf)

for timeIdx in range(totalTimeSteps):

    # 行番号（timeIdx）を直接指定して、該当する時間のデータを1行ずつ取得
    rainRow = precipitationDf.iloc[timeIdx]
    windRow = windDf.iloc[timeIdx]
    tempRow = temperatureDf.iloc[timeIdx]

    # [浜松, 磐田, 天竜] の順番で観測値の配列を作成
    rainValues = np.array([rainRow["hamamatsu"], rainRow["iwata"], rainRow["tenryu"]])
    windValues = np.array([windRow["hamamatsu"], windRow["iwata"], windRow["tenryu"]])
    tempValues = np.array([tempRow["hamamatsu"], tempRow["iwata"], tempRow["tenryu"]])

    # 100万点の一括IDW補間計算（行列とベクトルの内積）
    interpolatedRain = np.dot(normalizedWeights, rainValues)
    interpolatedWind = np.dot(normalizedWeights, windValues)
    interpolatedTemp = np.dot(normalizedWeights, tempValues)

    # C++側が最速で読み込める「3列だけ」のDataFrameを作成
    currentWeatherData = pd.DataFrame(
        {
            "rain": interpolatedRain,
            "windspeed": interpolatedWind,
            "tempre": interpolatedTemp,
        }
    )

    # ファイル名にインデックス（weather_0.csv, weather_1.csv...）を付与して保存
    outputFileName = f"weather_{timeIdx}.csv"
    currentWeatherData.to_csv(outputFileName, index=False)
    print(f"{outputFileName} を出力しました。")

print("すべてのCSV出力が完了しました。")
