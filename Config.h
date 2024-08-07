/* Header file for Config.c */

/* ---------------------------- */
/* 実行環境の設定を行うファイル */
/* ---------------------------- */

#define INFINITE_LOOP_MODE 0
// ステージ1-8の無限周回モードを使用する場合は1を入力
// 4回周回後、ドローンを起動する場合は0

#define GYRO_SETTING 1
// ジャイロ操作をONにしている場合は1、OFFの場合は0を入力

#define SENSITIVITY 5
// 設定感度の値を記述
// 私の場合は操作感度を最大値の5にしているので、そのまま5と記述している

#define REVERSE_LR 0
// Rスティックの上下操作をリバースにしている場合は1を入力、ノーマルなら0

#define REVERSE_UD 0
// Rスティックの左右操作をリバースにしている場合は1を入力、ノーマルなら0

#define SOFT_TYPE 0
// DL版なら0、カセット版なら1