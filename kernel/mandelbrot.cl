/* (C) Copyright 2018 by Roland Bernard. All rights reserved. */
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void computeColors(__global uchar3* buffer,
                            double2 delta,
                            double2 topLeft,
                            uint2 res,
                            uint iterationen,
                            uint samples)
{
    double2 c;
    double2 z;
    double2 tmpZ;
    uint2 s;
    uint i;
    float smooth;
    uint e;
    float3 tmp = (float3)(0.0, 0.0, 0.0);
    double2 preC = topLeft + delta * (double2)(get_global_id(0)%res.x, get_global_id(0)/res.x);


    for(s.x = 0; s.x < samples; s.x++)
        for(s.y = 0; s.y < samples; s.y++)
        {
            z = (double2)(0.0, 0.0);
            tmpZ = z;

            c = preC + delta / samples * convert_double2(s);

            for(i = 0; i < iterationen && tmpZ.y + tmpZ.x < 4; i++)
            {
                z.y = z.x*z.y;
                z.y += z.y + c.y;
                z.x = tmpZ.x - tmpZ.y + c.x;
                tmpZ.x = z.x*z.x;
                tmpZ.y = z.y*z.y;
            }

            for (e=0; e<4; ++e)
            {
                z.y = 2*z.x*z.y + c.y;
                z.x = tmpZ.x - tmpZ.y + c.x;
                tmpZ.x = z.x*z.x;
                tmpZ.y = z.y*z.y;
            }

            if(i < iterationen)
            {
                smooth = (i + 1 - (.69314718055994530941723212145817656807550013436026f / native_sqrt(convert_float(tmpZ.y + tmpZ.x)) / .69314718055994530941723212145817656807550013436026f));

                tmp += (float3)((native_sin(0.01f * smooth + 1) * 230 + 25)  / samples / samples,
                                (native_sin(0.013f * smooth + 2) * 230 + 25)  / samples / samples,
                                (native_sin(0.016f * smooth + 4) * 230 + 25) / samples / samples);
            }

        }

        buffer[get_global_id(0)] = convert_uchar3(tmp);
}
