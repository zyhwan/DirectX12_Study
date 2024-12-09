//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	//���� ���, ī�޶� ���, ���� ����� �ϳ��� ������ ��Ľ�
	float4x4 gWorldViewProj; 
	//��� gTime �߰�
    float gTime;
	
    float4 gPulseColor;
};

//�Է� �Ű�����
struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

//��� �Ű�����
struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};


//���� ���̴�
VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// ���� ��ȯ
    vin.PosL.xy += 0.5f * sin(vin.PosL.x) * sin(3.0f * gTime);
    vin.PosL.z *= 0.6f + 0.4f * sin(2.0f * gTime);
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// ���� ������ �״�� �ȼ� ���̴��� �����Ѵ�.
    vout.Color = vin.Color;
    
    return vout;
}

//�ȼ� ���̴�
//�ȼ� ���̴��� ��°��� �ϳ��� 4���� ���� ��
float4 PS(VertexOut pin) : SV_Target
{
    //pin.Color.z = smoothstep(0, 1, cos(gTime));
	//// ���� ���� 0���� ������ ����Ѵ�.
    //clip(pin.Color.r - 0.5f);
	//
    //return pin.Color;
	
	
    const float pi = 3.14159;

	// ���� �Լ��� �̿��ؼ�, �ð��� ���� [0, 1] �������� �����ϴ� ���� ���Ѵ�.
    float s = 0.5f * sin(2 * gTime - 0.25 * pi) + 0.5f;
	
	// �Ű����� s�� �����ؼ� pin.Color�� gPulseColor ���̸� �Ų����� ������ ���� ���Ѵ�.
    float4 c = lerp(pin.Color, gPulseColor, s);
	
    return c;
}


