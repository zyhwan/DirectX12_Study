//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	//월드 행렬, 카메라 행렬, 투영 행렬을 하나로 결합한 행렬식
	float4x4 gWorldViewProj; 
	//상수 gTime 추가
    float gTime;
	
    float4 gPulseColor;
};

//입력 매개변수
struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

//출력 매개변수
struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};


//정점 셰이더
VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// 투영 변환
    vin.PosL.xy += 0.5f * sin(vin.PosL.x) * sin(3.0f * gTime);
    vin.PosL.z *= 0.6f + 0.4f * sin(2.0f * gTime);
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// 정점 색상을 그대로 픽셀 셰이더에 전달한다.
    vout.Color = vin.Color;
    
    return vout;
}

//픽셀 셰이더
//픽셀 셰이더의 출력값은 하나의 4차원 색상 값
float4 PS(VertexOut pin) : SV_Target
{
    //pin.Color.z = smoothstep(0, 1, cos(gTime));
	//// 인자 값이 0보다 작으면 폐기한다.
    //clip(pin.Color.r - 0.5f);
	//
    //return pin.Color;
	
	
    const float pi = 3.14159;

	// 사인 함수를 이용해서, 시간에 따라 [0, 1] 구간에서 진동하는 값을 구한다.
    float s = 0.5f * sin(2 * gTime - 0.25 * pi) + 0.5f;
	
	// 매개변수 s에 기초해서 pin.Color와 gPulseColor 사이를 매끄럽게 보간한 값을 구한다.
    float4 c = lerp(pin.Color, gPulseColor, s);
	
    return c;
}


