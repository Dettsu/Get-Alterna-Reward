/* Header file for Step.c */

#include <stdint.h>

/* ボタンの記述について Buttons_t で定義 */
typedef enum {
	L_UP,
	L_DOWN,
	L_LEFT,
	L_RIGHT,
	R_UP,
	R_DOWN,
	R_LEFT,
	R_RIGHT,
	TOP,
	BOTTOM,
	LEFT,
	RIGHT,
	A,
	B,
	X,
	Y,
	L,
	R,
	ZL,
	ZR,
	MINUS,
	PLUS,
	TRIGGERS,
	AIM_SHOT,
	AIM_MAP,
	JUMP,
	NOTHING,
	END
} Buttons_t;

/* コマンドの記述方法についての定義 */
typedef struct {
	Buttons_t button; // Buttons_t で定義された文字列から任意のものを 変数 button に代入するため定義
	uint16_t duration; // 時間的な間隔をフレーム単位で示す変数 duration の定義
} command; // これを新たに command 型として定義

/* Step.c 内の関数について定義 */
command ConnectController(int index);
command SyncController(int index);
command GoToAlterna(int index);
command OpenOption(int index);
command TurnOffGyro(int index);
command SetSensitivity(int index, int mode);
command JumpToStage(int index);
command EnterStage(int index);
command ClearStage(int index);
command LunchDrone(int index);
command ResetGyroSetting(int index);
command BackToSplatsville(int index);