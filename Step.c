/* ---------------------------------------------- */
/* スプラトゥーン3 オルタナのドローン起動を自動化 */
/* 									by Dettsu3420 */
/* ---------------------------------------------- */

#include "Step.h"

/* コントローラーとして Nintendo Switchに接続後、少し待機させる [0 - 2] */
command ConnectController(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ NOTHING,   30 }, // [0]
		{ A,         10 }, // [1]
		{ NOTHING,   60 }, // [2]
		{ END,        0 }
	};

	return step[index];
}

/* コントローラーとしてNintendo Switchに認識させる [3 - 6] */
command SyncController(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ TRIGGERS,  10 }, // [3]
		{ NOTHING,   30 }, // [4]
		{ A,         10 }, // [5]
		{ NOTHING,   60 }, // [6]
		{ END,        0 }
	};

	return step[index];
}

/* 広場からオルタナに移動する [7 - 12] */
command GoToAlterna(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ X,		 10 }, // [7]
		{ NOTHING,   10 }, // [8]
		{ BOTTOM,     5 }, // [9]
		{ NOTHING,   10 }, // [10]
		{ A,         10 }, // [11]
		{ NOTHING,  540 }, // [12]
		{ END,		180 }
	};

	return step[index];
}

/* メニューからオプションを開く [13 - 18] */
command OpenOption(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ X,         10 }, // [13]
		{ NOTHING,   10 }, // [14]
		{ L,          5 }, // [15]
		{ NOTHING,    5 }, // [16]
		{ A,          5 }, // [17]
		{ NOTHING,    5 }, // [18]
		{ END,		  0 }
	};

	return step[index];
}
	
/* ジャイロ操作をOFFに設定する [19 - 24] */
command TurnOffGyro(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ TOP,        5 }, // [19]
		{ NOTHING,    5 }, // [20]
		{ A,          5 }, // [21]
		{ NOTHING,    5 }, // [22]
		{ BOTTOM,     5 }, // [23]
		{ NOTHING,    5 }, // [24]
		{ END,		  0 }
	};

	return step[index];
}

/* 操作感度をプログラム用に最適化・元の操作感度に復元 [25 - 26] */
command SetSensitivity(int index, int mode) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ END,		  0 }
	};

	static const command step_L[] = {
		{ LEFT,		  4 }, // [25]
		{ NOTHING,	  4 }, // [26]
		{ END,		  0 }
	};

	static const command step_R[] = {
		{ RIGHT,	  4 }, // [25]
		{ NOTHING,	  4 }, // [26]
		{ END,		  0 }
	};

	if (mode) {
		return step_L[index];
	} else {
		return step_R[index];
	}
}

/* ステージ1-8のヤカンへスーパージャンプ [27 - 42] */
command JumpToStage(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ L,         10 }, // [27]
		{ NOTHING,   10 }, // [28]
		{ L,         10 }, // [29]
		{ NOTHING,   10 }, // [30]
		{ A,         10 }, // [31]
		{ NOTHING,   10 }, // [32]
		{ TOP,        5 }, // [33]
		{ NOTHING,    5 }, // [34]
		{ TOP,        5 }, // [35]
		{ NOTHING,    5 }, // [36]
		{ TOP,        5 }, // [37]
		{ NOTHING,   10 }, // [38]
		{ A,         10 }, // [39]
		{ NOTHING,   10 }, // [40]
		{ A,         10 }, // [41]
		{ NOTHING,  330 }, // [42]
		{ END,		  0 }
	};

	return step[index];
}

/* ZLボタンを長押ししてヤカンに入る [43 - 44] */
command EnterStage(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ ZL,        40 }, // [43]
		{ NOTHING,  420 }, // [44]
		{ END,		120 }
	};

	return step[index];
}

/* ステージ1-8をクリアする [45 - 52] （視点移動は配列外で実行） */
command ClearStage(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ RIGHT,      5 }, // [45]
		{ NOTHING,   10 }, // [46]
		{ A,          5 }, // [47]
		{ L_UP,      85 }, // [48]
		{ A,          5 }, // [49]
		{ NOTHING,	145 }, // [50]
		{ ZR,		 45 }, // [51]
		{ AIM_SHOT,	 30 }, // 試作段階
		{ NOTHING, 1200 }, // [52]
		{ END,		  0 }
	};

	return step[index];
}

/* ドローンを起動してアイテムを探してきてもらう [53 - 69] */
command LunchDrone(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ X,         10 }, // [53]
		{ NOTHING,    5 }, // [54]
		{ AIM_MAP,	 20 }, // [55]
		{ A,          5 }, // [56]
		{ NOTHING,   10 }, // [57]
		{ A,          5 }, // [58]
		{ NOTHING,  175 }, // [59]
		{ R_LEFT,	 23 }, // [60]
		{ L_UP,     105 }, // [61]
		{ JUMP,		 20 },
		{ L_UP,		 75 },
	//	{ A,          5 }, // [62]
		{ NOTHING,  180 }, // [63]
		{ TOP,        5 }, // [64]
		{ NOTHING,   10 }, // [65]
	//	{ A,          5 }, // [66]
		{ NOTHING,   15 }, // [67]
	//	{ MINUS,      5 }, // [68]
		{ NOTHING,   90 }, // [69]
		{ END,		  0 }
	};

	return step[index];
}
  
/* メニューからオプションを開く（再呼び出し） [70 - 75] */

/* 操作設定を元の状態に戻す（再呼び出し） [76 - 77] */

/* ジャイロ操作の設定をONに戻す [78 - 81] */
command ResetGyroSetting(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ TOP,        5 }, // [78]
		{ NOTHING,   10 }, // [79]
		{ A,          5 }, // [80]
		{ NOTHING,   10 }, // [81]
		{ END,		  0 }
	};

	return step[index];
}

/* バンカラ街へ戻る [82 - 86] */
command BackToSplatsville(int index) {

	/* 入力されるコマンドを配列 step[] に格納 */
	static const command step[] = {
		{ B,          5 }, // [82]
		{ NOTHING,   10 }, // [83]
		{ PLUS,       5 }, // [84]
		{ NOTHING,   10 }, // [85]
		{ A,          5 }, // [86]
		{ END,		  0 }
	};

	return step[index];
};