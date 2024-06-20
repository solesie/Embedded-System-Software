package com.example.minigamecontroller;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.util.Log;

public class MainActivity extends Activity {
	
	private static String TAG = "MainActivity";
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		Log.i(TAG, "onCreate()");
		Button game1Button = (Button) findViewById(R.id.game1Button);
		Button game2Button = (Button) findViewById(R.id.game2Button);
		game1Button.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View v) {
				Log.i(TAG, "b1 onClick()");
				Intent intent = new Intent(
						MainActivity.this,
						Game1Activity.class);
				startActivity(intent);
			}
		});
		game2Button.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick(View v){
				Log.i(TAG, "b2 onClick()");
				Intent intent = new Intent(
						MainActivity.this,
						Game2Activity.class);
				startActivity(intent);
			}
		});
	}

	@Override
	public void onDestroy(){
		super.onDestroy();
		Log.i(TAG, "onDestroy()");
	}
}
