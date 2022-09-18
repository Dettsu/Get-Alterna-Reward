/*
Nintendo Switch Fightstick - Proof-of-Concept

Based on the LUFA library's Low-Level Joystick Demo
	(C) Dean Camera
Based on the HORI's Pokken Tournament Pro Pad design
	(C) HORI

This project implements a modified version of HORI's Pokken Tournament Pro Pad
USB descriptors to allow for the creation of custom controllers for the
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the Pokken
Tournament Pro Pad as a Pro Controller. Physical design limitations prevent
the Pokken Controller from functioning at the same level as the Pro
Controller. However, by default most of the descriptors are there, with the
exception of Home and Capture. Descriptor modification allows us to unlock
these buttons for our use.
*/

/** \file
 *
 *  Main source file for the posts printer demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Joystick.h"

extern const uint8_t image_data[0x12c1] PROGMEM;

// Main entry point.
int main(void) {
	// We'll start by performing hardware and peripheral setup.
	SetupHardware();
	// We'll then enable global interrupts for our use.
	GlobalInterruptEnable();
	// Once that's done, we'll enter an infinite loop.
	for (;;)
	{
		// We need to run our task to process and deliver data for our IN and OUT endpoints.
		HID_Task();
		// We also need to run the main USB management task.
		USB_USBTask();
	}
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void) {
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);
	// We can then initialize our hardware and peripherals, including the USB stack.

	#ifdef ALERT_WHEN_DONE
	// Both PORTD and PORTB will be used for the optional LED flashing and buzzer.
	#warning LED and Buzzer functionality enabled. All pins on both PORTB and \
PORTD will toggle when printing is done.
	DDRD  = 0xFF; //Teensy uses PORTD
	PORTD =  0x0;
                  //We'll just flash all pins on both ports since the UNO R3
	DDRB  = 0xFF; //uses PORTB. Micro can use either or, but both give us 2 LEDs
	PORTB =  0x0; //The ATmega328P on the UNO will be resetting, so unplug it?
	#endif
	// The USB stack should be initialized last.
	USB_Init();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void) {
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void) {
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void) {
	// We can handle two control requests: a GetReport and a SetReport.

	// Not used here, it looks like we don't receive control request from the Switch.
}

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void) {
	// If the device isn't connected and properly configured, we can't do anything here.
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

	// We'll start with the OUT endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
	// We'll check to see if we received something on the OUT endpoint.
	if (Endpoint_IsOUTReceived())
	{
		// If we did, and the packet has data, we'll react to it.
		if (Endpoint_IsReadWriteAllowed())
		{
			// We'll create a place to store our data received from the host.
			USB_JoystickReport_Output_t JoystickOutputData;
			// We'll then take in that data, setting it up in our storage.
			while(Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL) != ENDPOINT_RWSTREAM_NoError);
			// At this point, we can react to this data.

			// However, since we're not doing anything with this data, we abandon it.
		}
		// Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
		Endpoint_ClearOUT();
	}

	// We'll then move on to the IN endpoint.
	Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
	// We first check to see if the host is ready to accept data.
	if (Endpoint_IsINReady())
	{
		// We'll create an empty report.
		USB_JoystickReport_Input_t JoystickInputData;
		// We'll then populate this report with what we want to send to the host.
		GetNextReport(&JoystickInputData);
		// Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
		while(Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL) != ENDPOINT_RWSTREAM_NoError);
		// We then send an IN packet on this endpoint.
		Endpoint_ClearIN();
	}
}

typedef enum {
	CONNECT_CONTROLLER,
	SYNC_CONTROLLER,
	GO_TO_ALTERNA,
	OPEN_OPTION,
	TURN_OFF_GYRO,
	SET_SENSITIVITY,
	JUMP_TO_STAGE,
	ENTER_STAGE,
	CLEAR_STAGE,
	GET_ITEM,
	RESET_SENSITIVITY,
	RESET_GYRO_SETTING,
	BACK_TO_SPLATSVILLE,
	DONE
} State_t;
State_t state = CONNECT_CONTROLLER;

#define ECHOES 2
int echoes = 0;
USB_JoystickReport_Input_t last_report;

int report_count = 0;
int clear_count = 0;
int sensitivity_count = 0;
// --------------------------------	   設定		--------------------------------
int gyro_setting = 1;
// ジャイロ操作をONにしている場合は1、OFFの場合は0を入力
int sensitivity = 10;
// 設定感度を2倍した値を記述
// 私の場合は操作感度を最大値の5にしているので、2倍して10を記述している
// --------------------------------	設定ここまで	--------------------------------
int timer = 0;
int flag = 0;
int portsval = 0;

// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t* const ReportData) {

	// Prepare an empty report
	memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));
	ReportData->LX = STICK_CENTER;
	ReportData->LY = STICK_CENTER;
	ReportData->RX = STICK_CENTER;
	ReportData->RY = STICK_CENTER;
	ReportData->HAT = HAT_CENTER;

	// Repeat ECHOES times the last report
	if (echoes > 0)
	{
		memcpy(ReportData, &last_report, sizeof(USB_JoystickReport_Input_t));
		echoes--;
		return;
	}

	// States and moves management
	switch (state)
	{
		case CONNECT_CONTROLLER: // Nintendo Switchに接続後、少し待機させる
			if (report_count > 100)
			{
				report_count = 0;
				// コントローラーを認識させる動作へ移行する
				state = SYNC_CONTROLLER;
			}
			else if (10 < report_count && report_count < 20)
			{
				// Aボタンを押し、Nintendo Switchに認識させる
				ReportData->Button |= SWITCH_A;
			}
			report_count++;
			break;
		case SYNC_CONTROLLER: // コントローラーとしてNintendo Switchに認識させる
			if (report_count > 100)
			{
				report_count = 0;
				// メニューからオプションを開く動作に移行する
				state = GO_TO_ALTERNA;
			}
			else if (20 < report_count && report_count < 30)
			{
				// LボタンおよびRボタンを同時押し
				ReportData->Button |= SWITCH_L | SWITCH_R;
			}
			else if (50 < report_count && report_count < 60)
			{
				// Aボタンを押し、Nintendo Switchに認識させる
				ReportData->Button |= SWITCH_A;
			}
			report_count++;
			break;
		case GO_TO_ALTERNA: // 広場からオルタナに移動する
			if (report_count > 550)
			{
				report_count = 0;
				state = OPEN_OPTION;
			}
			else if (25 < report_count && report_count < 30)
			{
				// Xボタンでメニューを開く
				ReportData->Button |= SWITCH_X;
			}
			else if (40 < report_count && report_count < 45)
			{
				// 十字キー下でオルタナにカーソルを合わせる
				ReportData->HAT = HAT_BOTTOM;
			}
			else if (50 < report_count && report_count < 55)
			{
				// Aボタンでオルタナに入る
				ReportData->Button |= SWITCH_A;
			}
			report_count++;
			break;
		case OPEN_OPTION: // メニューからオプションを開く
			if (report_count > 25)
			{
				report_count = 0;
				state = TURN_OFF_GYRO;
			}
			else if (0 < report_count && report_count < 5)
			{
				// Xボタンでメニューを開く
				ReportData->Button |= SWITCH_X;
			}
			else if (20 < report_count && report_count < 25)
			{
				// Lボタンでオプションに移動する
				ReportData->Button |= SWITCH_L;
			}
			report_count++;
			break;
		case TURN_OFF_GYRO: // ジャイロ操作をOFFに設定する
			if (report_count > 120)
			{
				report_count = 0;
				state = SET_SENSITIVITY;
			}
			else if (10 < report_count && report_count < 20)
			{
				// Aボタンを押す
				ReportData->Button |= SWITCH_A;
			}
			// これより下の(*)の箇所はジャイロ操作をOFFにする動作
			// ジャイロ操作がもともとOFFの場合は上で設定を変更
			else if (gyro_setting == 1 && 45 < report_count && report_count < 50)
			{
				// 十字上キーでジャイロ操作の欄に移動する(*)
				ReportData->HAT = HAT_TOP;
			}
			else if (gyro_setting == 1 && 75 < report_count && report_count < 85)
			{
				// Aボタンでジャイロ操作をOFFにする(*)
				ReportData->Button |= SWITCH_A;
			}
			else if (gyro_setting == 1 && 95 < report_count && report_count < 100)
			{
				// 十字下キーで操作感度の欄に移動する(*)
				ReportData->HAT = HAT_BOTTOM;
			}
			// ここまでジャイロ操作をOFFにする動作
			report_count++;
			break;
		case SET_SENSITIVITY: // 操作感度を0にしてプログラム用に最適化
			if (flag == 1)
			{
				report_count = 0;
				sensitivity_count = 0;
				state = JUMP_TO_STAGE;
			}
			else if (sensitivity < 0 && sensitivity_count != sensitivity && 10 * sensitivity_count < report_count && report_count < 10 * sensitivity_count + 5)
			{
				// 十字キー右を押す
				ReportData->HAT = HAT_RIGHT;
				sensitivity_count--;
				timer = report_count;
			}
			else if (sensitivity > 0 && sensitivity_count != sensitivity && 10 * sensitivity_count < report_count && report_count < 10 * sensitivity_count + 5)
			{
				// 十字キー左を押す
				ReportData->HAT = HAT_LEFT;
				sensitivity_count++;
				timer = report_count;
			}
			if (sensitivity == sensitivity_count && timer + 10 < report_count && report_count < timer + 15)
			{
				// Bボタンで操作感度の設定を終える
				ReportData->Button |= SWITCH_B;
				flag = 1;
			}
			report_count++;
			break;
		case JUMP_TO_STAGE: // ステージ1-8のヤカンへスーパージャンプで移動
			if (report_count > 320)
			{
				report_count = 0;
				state = ENTER_STAGE;
			}
			else if (0 < report_count && report_count < 10)
			{
				// Lボタンを押す(1)
				ReportData->Button |= SWITCH_L;
			}
			else if (20 < report_count && report_count < 30)
			{
				// Lボタンを押す(2)
				ReportData->Button |= SWITCH_L;
			}
			else if (50 < report_count && report_count < 55)
			{
				// Aボタンを押す
				ReportData->Button |= SWITCH_A;
			}
			else if (70 < report_count && report_count < 75)
			{
				// 十字キー上を押す(1)
				ReportData->HAT = HAT_TOP;
			}
			else if (80 < report_count && report_count < 85)
			{
				// 十字キー上を押す(2)
				ReportData->HAT = HAT_TOP;
			}
			else if (90 < report_count && report_count < 95)
			{
				// 十字キー上を押す(3)
				ReportData->HAT = HAT_TOP;
			}
			else if (100 < report_count && report_count < 105)
			{
				// Aボタンを押す
				ReportData->Button |= SWITCH_A;
			}
			else if (110 < report_count && report_count < 115)
			{
				// Aボタンを押す
				ReportData->Button |= SWITCH_A;
			}
			report_count++;
			break;
		case ENTER_STAGE: // ZLボタンを長押ししてヤカンに入る
			if (report_count == 420)
			{
				report_count = 0;
				state = CLEAR_STAGE;
			}
			else
			{
				// ZLボタンを押し続ける
				ReportData->Button |= SWITCH_ZL;
			}
			report_count++;
			break;
		case CLEAR_STAGE: // ステージ1-8をクリアする
			if (report_count > 1440 && clear_count > 3)
			{
				report_count = 0;
				state = GET_ITEM;
			}
			else if (report_count > 1450)
			{
				report_count = 0;
				clear_count++;
				state = ENTER_STAGE;
			}
			else if (0 < report_count && report_count < 5)
			{
				// 十字キー右ボタンで4Kスコープにカーソルを合わせる
				ReportData->HAT = HAT_RIGHT;
			}
			else if (15 < report_count && report_count < 20)
			{
				// Aボタンでブキを選択
				ReportData->Button |= SWITCH_A;
			}
			else if (20 < report_count && report_count < 105)
			{
				// Lスティックを上に倒し続ける
				ReportData->LY = STICK_MIN;
			}
			else if (105 < report_count && report_count < 110)
			{
				// Aボタンでミッションを開始
				ReportData->Button |= SWITCH_A;
			}
			else if (230 < report_count && report_count < 320)
			{
				// ZRボタン長押しでフルチャージ
				ReportData->Button |= SWITCH_ZR;
			}
			if (270 < report_count && report_count < 275)
			{
				// 視点を上にずらす
				ReportData->RY = 48;
			}
			if (290 < report_count && report_count < 295)
			{
				// 視点を左にずらす
				ReportData->RX = 88;
			}
			if (50 < report_count && report_count < 55)
			{
				// Aボタンでブキを選択
				ReportData->Button |= SWITCH_Y;
			}
			report_count++;
			break;
		case GET_ITEM: // ドローンを起動してアイテムを探してきてもらう
			if (report_count > 700)
			{
				report_count = 0;
				state = RESET_SENSITIVITY;
			}
			else if (0 < report_count && report_count < 10 )
			{
				ReportData->Button |= SWITCH_X;
			}
			else if (20 < report_count && report_count < 40)
			{
				// シオカラキャンプに照準を合わせる
				ReportData->LX = STICK_MIN;
				ReportData->LY = 192;
			}
			else if (50 < report_count && report_count < 55)
			{
				// Aボタンでシオカラキャンプを選択
				ReportData->Button |= SWITCH_A;
			}
			else if (70 < report_count && report_count < 75)
			{
				// Aボタンでシオカラキャンプにジャンプ
				ReportData->Button |= SWITCH_A;
			}
			else if (240 < report_count && report_count < 260)
			{
				// Rスティックを左に倒してドローンの方向に視点を向ける
				ReportData->RX = 24;
			}
			else if (260 < report_count && report_count < 455)
			{
				// Lスティックを上に倒しこんでドローンに移動
				ReportData->LY = STICK_MIN;
			}
			else if (455 < report_count && report_count < 460)
			{
				// Aボタンでドローンを起動
				ReportData->Button |= SWITCH_A;
			}
			else if (630 < report_count && report_count < 635)
			{
				// 十字キー上でカーソルを合わせる
				ReportData->HAT = HAT_TOP;
			}
			else if (640 < report_count && report_count < 645)
			{
				// Aボタンでドローンを起動
				ReportData->Button |= SWITCH_A;
			}
			else if (660 < report_count && report_count < 665)
			{
				// マイナスボタンでスキップ
				ReportData->Button |= SWITCH_MINUS;
			}
			if (365 < report_count && report_count < 385)
			{
				// ドローンまでの道のりの段差を飛び越える
				ReportData->Button |= SWITCH_B;
			}
			report_count++;
			break;
		case RESET_SENSITIVITY: // オプション画面で操作設定を元の状態に戻す
			if (sensitivity == -10 && report_count > 130 || sensitivity == 10 && report_count > 130 || sensitivity_count == sensitivity)
			{
				report_count = 0;
				state = RESET_GYRO_SETTING;
			}
			else if (0 < report_count && report_count < 5 )
			{
				// Xボタンでメニューを開く
				ReportData->Button |= SWITCH_X;
			}
			else if (15 < report_count && report_count < 20)
			{
				// Lボタンでオプション画面に移動
				ReportData->Button |= SWITCH_L;
			}
			else if (30 < report_count && report_count < 35)
			{
				// Aボタンで操作感度の設定に入る
				ReportData->Button |= SWITCH_A;
			}
			else if (sensitivity == -10 && 45 < report_count && report_count < 125)
			{
				ReportData->LX = STICK_MIN;
			}
			else if (sensitivity == 10 && 45 < report_count && report_count < 125)
			{
				ReportData->LX = STICK_MAX;
			}
			else if (sensitivity < 0 && sensitivity_count != sensitivity && 10 * sensitivity_count + 45 < report_count && report_count < 10 * sensitivity_count + 50)
			{
				// 十字キー左を押す
				ReportData->HAT = HAT_LEFT;
				sensitivity_count--;
			}
			else if (sensitivity > 0 && sensitivity_count != sensitivity && 10 * sensitivity_count + 45 < report_count && report_count < 10 * sensitivity_count + 50)
			{
				// 十字キー右を押す
				ReportData->HAT = HAT_RIGHT;
				sensitivity_count++;
			}
			report_count++;
			break;
		case RESET_GYRO_SETTING: // ジャイロ操作の設定を元の状態に戻す
			if (report_count > 50)
			{
				report_count = 0;
				state = BACK_TO_SPLATSVILLE;
			}
			else if (10 < report_count && report_count < 15 )
			{
				// 十字キー上でジャイロ操作の設定に移る
				ReportData->HAT = HAT_TOP;
			}
			else if (20 < report_count && report_count < 25)
			{
				// Aボタンでジャイロ操作をONに戻す
				ReportData->Button |= SWITCH_A;
			}
			else if (30 < report_count && report_count < 35)
			{
				// Bボタンで操作感度の設定を終える
				ReportData->Button |= SWITCH_B;
			}
			report_count++;
			break;
		case BACK_TO_SPLATSVILLE: // ジャイロ操作の設定を元の状態に戻す
			if (report_count > 100)
			{
				report_count = 0;
				state = DONE;
			}
			else if (0 < report_count && report_count < 5 )
			{
				// プラスボタンでバンカラ街へ戻る
				ReportData->Button |= SWITCH_PLUS;
			}
			else if (15 < report_count && report_count < 20)
			{
				// Aボタンで決定
				ReportData->Button |= SWITCH_A;
			}
			report_count++;
			break;
		case DONE:
			#ifdef ALERT_WHEN_DONE
			portsval = ~portsval;
			PORTD = portsval; //flash LED(s) and sound buzzer if attached
			PORTB = portsval;
			_delay_ms(250);
			#endif
			return;
	}

	// Prepare to echo this report
	memcpy(&last_report, ReportData, sizeof(USB_JoystickReport_Input_t));
	echoes = ECHOES;

}
