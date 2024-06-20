package com.example.minigamecontroller;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class BackPopupActivity extends Activity {
	private static String TAG = "BackPopupActivity";
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		Log.i(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_back_popup);
		
		Button resumeButton = (Button) findViewById(R.id.resumeButton);
		Button exitButton = (Button) findViewById(R.id.exitButton);
		Button restartButton = (Button) findViewById(R.id.restartButton);
		
		// resumeButton click listener
		resumeButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.i(TAG, "resumeButton clicked");
				Intent intent = new Intent(BackPopupActivity.this, Game1Activity.class);
				// reuse current Game1Activity
				intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
				intent.putExtra("RESUME", true);
				startActivity(intent);
				finish();
			}
		});

		// closeButton click listener
		exitButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.i(TAG, "closeButton clicked");
				Intent intent = new Intent(BackPopupActivity.this, Game1Activity.class);
				// reuse current Game1Activity
				intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
				intent.putExtra("EXIT", true);
				startActivity(intent);
				finish();
			}
		});
		
		// restart click listener
		restartButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Log.i(TAG, "restart clicked");
				Intent intent = new Intent(BackPopupActivity.this, Game1Activity.class);
				// reuse current Game1Activity
				intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
				intent.putExtra("RESTART", true);
				startActivity(intent);
				finish();
			}
		});
	}
}
