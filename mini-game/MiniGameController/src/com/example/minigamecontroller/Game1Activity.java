package com.example.minigamecontroller;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class Game1Activity extends Activity implements SurfaceHolder.Callback {
	private native void nativeOnCreate();
	private native void nativeOnResume();
	private native void nativeOnPause();
	private native void nativeOnDestroy();
	private native void nativeSetSurface(Surface surface);
	private native void nativeRestartGame();
	private native boolean nativeWaitBackInterrupt();
	
	private static String TAG = "Game1Activity";
	
	class BackInterruptDetector extends Thread{
		BackInterruptDetector(){}
		
		public void run(){
			Log.i(TAG, "Interrupt Dectector started");
			// Blocking manner
			if(nativeWaitBackInterrupt()){
				Log.i(TAG, "Waked up by interrupt");
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

		SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surfaceview1);
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
		else if (getIntent().getBooleanExtra("RESUME", false)) {
			Log.i(TAG, "onNewIntent(RESUME)");
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
	
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		nativeSetSurface(holder.getSurface());
	}
	
	@Override
	public void surfaceCreated(SurfaceHolder holder) {}
	
	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		nativeSetSurface(null);
	}
}
