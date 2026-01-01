package com.lakesnes.emulator;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class EmulatorActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    
    private static final int REQUEST_SELECT_ROM = 1001;
    
    private SurfaceView surfaceView;
    private Button btnPause, btnReset, btnFastForward, btnStateSave, btnStateLoad, btnMenu;
    
    // Native method declarations
    public native void nativeSurfaceChanged(int width, int height);
    public native void nativeSurfaceDestroyed();
    public native void nativeTouchEvent(int action, float x, float y);
    public native void nativeButtonPressed(int button, boolean pressed);
    public native void nativeLoadRom(String path);
    public native byte[] nativeReadFileFromUri(String uri);
    public native void nativePause();
    public native void nativeResume();
    public native void nativeReset();
    public native void nativeSetFastForward(boolean fastForward);
    public native void nativeSaveState();
    public native void nativeLoadState();
    
    static {
        System.loadLibrary("lakesnes");
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Fullscreen mode
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        
        setContentView(R.layout.activity_emulator);
        
        // Initialize UI elements
        initUI();
        
        // Initialize surface view
        surfaceView = findViewById(R.id.surface_view);
        SurfaceHolder holder = surfaceView.getHolder();
        holder.addCallback(this);
        
        // Handle intent (for opening ROM files)
        handleIntent(getIntent());
    }
    
    private void initUI() {
        btnPause = findViewById(R.id.btn_pause);
        btnReset = findViewById(R.id.btn_reset);
        btnFastForward = findViewById(R.id.btn_fast_forward);
        btnStateSave = findViewById(R.id.btn_state_save);
        btnStateLoad = findViewById(R.id.btn_state_load);
        btnMenu = findViewById(R.id.btn_menu);
        
        btnPause.setOnClickListener(v -> togglePause());
        btnReset.setOnClickListener(v -> nativeReset());
        btnFastForward.setOnClickListener(v -> toggleFastForward());
        btnStateSave.setOnClickListener(v -> nativeSaveState());
        btnStateLoad.setOnClickListener(v -> nativeLoadState());
        btnMenu.setOnClickListener(v -> showMenu());
        
        // Gamepad buttons
        setupGamepadButtons();
    }
    
    private void setupGamepadButtons() {
        int[] buttonIds = {
            R.id.btn_up, R.id.btn_down, R.id.btn_left, R.id.btn_right,
            R.id.btn_a, R.id.btn_b, R.id.btn_x, R.id.btn_y,
            R.id.btn_l, R.id.btn_r, R.id.btn_select, R.id.btn_start
        };
        
        String[] buttonNames = {"up", "down", "left", "right", "a", "b", "x", "y", "l", "r", "select", "start"};
        
        for (int i = 0; i < buttonIds.length; i++) {
            Button button = findViewById(buttonIds[i]);
            final int buttonIndex = i;
            final String buttonName = buttonNames[i];
            
            button.setOnTouchListener((v, event) -> {
                boolean pressed = event.getAction() == android.view.MotionEvent.ACTION_DOWN;
                nativeButtonPressed(buttonIndex, pressed);
                return true;
            });
        }
    }
    
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        // Surface is ready, but we wait for surfaceChanged to get dimensions
    }
    
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        nativeSurfaceChanged(width, height);
    }
    
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        nativeSurfaceDestroyed();
    }
    
    private void togglePause() {
        // Toggle pause state
        nativePause();
    }
    
    private void toggleFastForward() {
        // Toggle fast forward
        boolean currentState = btnFastForward.getTag() instanceof Boolean ? (Boolean) btnFastForward.getTag() : false;
        boolean newState = !currentState;
        nativeSetFastForward(newState);
        btnFastForward.setTag(newState);
        btnFastForward.setText(newState ? "FF+" : getString(R.string.emu_fast_forward));
    }
    
    private void showMenu() {
        // Show options menu or open ROM selector
        selectRom();
    }
    
    private void selectRom() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        
        String[] mimeTypes = {"application/x-snes-rom", "application/zip", "application/x-zip-compressed"};
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        
        startActivityForResult(intent, REQUEST_SELECT_ROM);
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        
        if (requestCode == REQUEST_SELECT_ROM && resultCode == RESULT_OK && data != null) {
            Uri uri = data.getData();
            if (uri != null) {
                getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                nativeLoadRom(uri.toString());
            }
        }
    }
    
    private void handleIntent(Intent intent) {
        if (intent != null && Intent.ACTION_VIEW.equals(intent.getAction())) {
            Uri uri = intent.getData();
            if (uri != null) {
                // Grant persistent permission for this URI
                getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                nativeLoadRom(uri.toString());
            }
        }
    }
    
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        // Handle orientation change if needed
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        nativePause();
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        nativeResume();
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        // Clean up native resources if needed
    }
    
    /**
     * Read file content from a content:// URI
     * @param uri The content:// URI to read from
     * @return byte array containing the file content, or null if failed
     */
    public byte[] nativeReadFileFromUri(String uri) {
        try {
            Uri parsedUri = Uri.parse(uri);
            java.io.InputStream inputStream = getContentResolver().openInputStream(parsedUri);
            if (inputStream == null) {
                LOGE("Failed to open input stream for URI: %s", uri);
                return null;
            }
            
            java.io.ByteArrayOutputStream byteArrayOutputStream = new java.io.ByteArrayOutputStream();
            byte[] buffer = new byte[8192];
            int bytesRead;
            int totalBytes = 0;
            
            while ((bytesRead = inputStream.read(buffer)) != -1) {
                byteArrayOutputStream.write(buffer, 0, bytesRead);
                totalBytes += bytesRead;
            }
            
            inputStream.close();
            LOGI("Successfully read %d bytes from URI: %s", totalBytes, uri);
            return byteArrayOutputStream.toByteArray();
        } catch (Exception e) {
            LOGE("Failed to read file from URI %s: %s", uri, e.getMessage());
            e.printStackTrace();
            return null;
        }
    }
    
    // Simple logging helpers
    private void LOGI(String format, Object... args) {
        android.util.Log.i("LakeSnes", String.format(format, args));
    }
    
    private void LOGE(String format, Object... args) {
        android.util.Log.e("LakeSnes", String.format(format, args));
    }
}