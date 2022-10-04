#ifdef VISION_MAP_ID
	float war_fog = 0.0;
	war_fog += sample_map(VISION_MAP_ID, coordw.xz / vec2(256)).r * 2;
	war_fog += sample_map(VISION_MAP_ID, (coordw.xz + vec2(-1, 0)) / vec2(256)).r;
	war_fog += sample_map(VISION_MAP_ID, (coordw.xz + vec2(+1, 0)) / vec2(256)).r;
	war_fog += sample_map(VISION_MAP_ID, (coordw.xz + vec2(0, +1)) / vec2(256)).r;
	war_fog += sample_map(VISION_MAP_ID, (coordw.xz + vec2(0, -1)) / vec2(256)).r;
	war_fog /= 6.0;
	ret *= mix(0.2, 1.0, war_fog);
#endif
