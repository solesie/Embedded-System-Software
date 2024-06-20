package com.example.minigamecontroller;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;

public class Game1Activity extends Activity implements SurfaceHolder.Callback {
	private static native void nativeOnCreate();
	private static native void nativeOnResume();
	private static native void nativeOnPause();
	private static native void nativeOnDestroy();
	private static native void nativeSetSurface(Surface surface);
	private static native void nativeRestartGame();
	private static native boolean nativeWaitBackInterrupt();
	
	private static String TAG = "Game1Activity";
	
	class BackInterruptDetector extends Thread{
		BackInterruptDetector(){}
		
		public void run(){
			Log.i(TAG, "Interrupt Dectector started");
			// Blocking manner
			if(nativeWaitBackInterrupt()){
				Log.i(TAG, "Waked up by interr");
				// wake up
				Intent intent = new Intent(Game1Activity.this, BackPopupActivity.class);
				startActivity(intent);
				overridePendingTransition(0, 0);
			}
			else{
				Log.i(TAG, "Waked up by pause");
			}
		}
	}
	private BackInterruptDetector backInterruptDetector;
	
	static {
		System.loadLibrary("mini-game");
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_game1);
		Log.i(TAG, "onCreate()");

		SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surfaceview);
		surfaceView.getHolder().addCallback(this);
		
		nativeOnCreate();
	}

	@Override
	protected void onResume() {
		super.onResume();
		Log.i(TAG, "onResume()");
		
		backInterruptDetector = new BackInterruptDetector();
		backInterruptDetector.setDaemon(true);
		backInterruptDetector.start();
		
		nativeOnResume();
	}

	@Override
	protected void onNewIntent(Intent intent) {
		super.onNewIntent(intent);
		setIntent(intent);
		if (getIntent().getBooleanExtra("EXIT", false)) {
			Log.i(TAG, "onNewIntent(EXIT)");
			finish();
		}
		else if(getIntent().getBooleanExtra("RESTART", false)){
			Log.i(TAG, "onNewIntent(RESTART)");
			nativeRestartGame();
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
		Log.i(TAG, "onPause()");
		nativeOnPause();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		Log.i(TAG, "onDestroy()");
		nativeOnDestroy();
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		nativeSetSurface(holder.getSurface());
	}

	public void surfaceCreated(SurfaceHolder holder) {}

	public void surfaceDestroyed(SurfaceHolder holder) {
		nativeSetSurface(null);
	}
}
