package com.example.minigamecontroller;

import android.os.Bundle;
import android.util.Log;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import java.security.SecureRandom;

public class Game2Activity extends Activity implements SurfaceHolder.Callback {
	private native void nativeOnCreate();
	private native void nativeOnResume();
	private native void nativeOnPause();
	private native void nativeOnDestroy();
	private native void nativeSetSurface(Surface surface);
	private native void nativeRestartGame();
	private native boolean nativeWaitBackInterrupt();
	
	private static String TAG = "Game2Activity";
	
	private static int ROW = 15, COL = 20;
	@SuppressLint("TrulyRandom")
	private static SecureRandom secureRandom = new SecureRandom();
	
	class BackInterruptDetector extends Thread{
		BackInterruptDetector(){}
		
		public void run(){
			Log.i(TAG, "Interrupt Dectector started");
			// Blocking manner
			if(nativeWaitBackInterrupt()){
				Log.i(TAG, "Waked up by interrupt");
				// wake up
				Intent intent = new Intent(Game2Activity.this, BackPopupActivity.class);
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
	
	/**
	 * Initialize maze using Eller's Algorithm.
	 * It is proper to initialize maze in C(or C++), because I use OPENGL ES 2.0 and NDK,
	 * but I declare maze in Java for educational purpose.
	 */
	private int[][] generateMaze() {
		// initialize
		int[][] ret = new int[2*ROW + 1][2*COL + 1];
		int[][] profile = new int[ROW][COL];
		for(int i = 0; i < 2*ROW + 1; ++i){
			for(int j = 0; j < 2*COL + 1; ++j){
				if((i & 1) != 0 && (j & 1) != 0)
					ret[i][j] = 0;
				else
					ret[i][j] = 1;
			}
		}
		for(int i = 0; i < ROW; ++i)
			for(int j = 0; j < COL; ++j)
				profile[i][j] = i*COL + j;
		
		// Eller's Algorithm
		for(int r = 0; r < ROW - 1; r++){
			for (int c = 0; c < COL - 1; c++) {
				boolean remove = secureRandom.nextInt(10) < 4 ? true : false; // 40%
				if (remove && (profile[r][c] != profile[r][c + 1])) {
					profile[r][c + 1] = profile[r][c];
					ret[r * 2 + 1][c * 2 + 2] = 0;
				}
			}
			
			int profileS = 0;
			while(profileS < COL){
				int nextProfileS;
				for(nextProfileS = profileS + 1; nextProfileS < COL; ++nextProfileS)
					if(profile[r][profileS] != profile[r][nextProfileS])
						break;
				// Connect at least one is necessary.
				int connectionIdx = profileS + secureRandom.nextInt(nextProfileS - profileS);
				profile[r + 1][connectionIdx] = profile[r][connectionIdx];
				ret[r * 2 + 2][connectionIdx * 2 + 1] = 0;
				
				for(int s = profileS; s < nextProfileS; ++s){
					boolean remove = secureRandom.nextInt(10) < 2 ? true : false; // 20%
					if(remove){
						profile[r + 1][connectionIdx] = profile[r][connectionIdx];
						ret[r * 2 + 2][connectionIdx * 2 + 1] = 0;
					}
				}
				profileS = nextProfileS;
			}
		}
		for (int c = 0; c < COL - 1; c++) 
			ret[(ROW - 1) * 2 + 1][c * 2 + 2] = 0;
		return ret;
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_game2);
		Log.i(TAG, "onCreate()");
		
		SurfaceView surfaceView = (SurfaceView)findViewById(R.id.surfaceview2);
		surfaceView.getHolder().addCallback(this);
		
		nativeOnCreate();
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		Log.i(TAG, "onResume()");
		
//		backInterruptDetector = new BackInterruptDetector();
//		backInterruptDetector.setDaemon(true);
//		backInterruptDetector.start();
		
		nativeOnResume();
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
	public void surfaceCreated(SurfaceHolder holder) { }

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		nativeSetSurface(null);
	}
}
