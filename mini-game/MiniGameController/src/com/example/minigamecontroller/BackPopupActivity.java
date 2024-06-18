package com.example.minigamecontroller;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

public class BackPopupActivity extends Activity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_back_popup);
        
        Button resumeButton = (Button) findViewById(R.id.resumeButton);
        Button closeButton = (Button) findViewById(R.id.closeButton);
        
     // resumeButton 클릭 리스너 설정
        resumeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // 현재 팝업 액티비티 종료
                finish();
                overridePendingTransition(0, 0);
            }
        });

        // closeButton 클릭 리스너 설정
        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // Game1Activity를 종료하기 위해 Activity stack에서 제거
                Intent intent = new Intent(BackPopupActivity.this, Game1Activity.class);
                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
                intent.putExtra("EXIT", true);
                startActivity(intent);
                finish();
            }
        });
    }
}
