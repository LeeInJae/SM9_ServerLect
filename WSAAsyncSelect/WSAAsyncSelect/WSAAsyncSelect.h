#pragma once

HWND GetWindowHandle( );

LRESULT CALLBACK WndProc( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam );
void ProcessSocketIO( HWND hWnd , WPARAM wParam , LPARAM lParam );
void ServerSocket_Accept( SOCKET ServerSocket , HWND hWnd );
void StreamSocket_Write( SOCKET StreamSocket );
void StreamSocket_Read( SOCKET StreamSocket );
void TargetSocket_Close( SOCKET TargetSocket );