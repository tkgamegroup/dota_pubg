#ifdef VISION_MAP_ID
	float war_fog = 0.0;
	war_fog += sample_map(VISION_MAP_ID, world_pos.xz / vec2(256)).r * 2;
	war_fog += sample_map(VISION_MAP_ID, (world_pos.xz + vec2(-1, 0)) / vec2(256)).r;
	war_fog += sample_map(VISION_MAP_ID, (world_pos.xz + vec2(+1, 0)) / vec2(256)).r;
	war_fog += sample_map(VISION_MAP_ID, (world_pos.xz + vec2(0, +1)) / vec2(256)).r;
	war_fog += sample_map(VISION_MAP_ID, (world_pos.xz + vec2(0, -1)) / vec2(256)).r;
	war_fog += sample_map(VISION_MAP_ID, (world_pos.xz + vec2(-1, -1)) / vec2(256)).r * 0.7;
	war_fog += sample_map(VISION_MAP_ID, (world_pos.xz + vec2(+1, +1)) / vec2(256)).r * 0.7;
	war_fog += sample_map(VISION_MAP_ID, (world_pos.xz + vec2(-1, +1)) / vec2(256)).r * 0.7;
	war_fog += sample_map(VISION_MAP_ID, (world_pos.xz + vec2(+1, -1)) / vec2(256)).r * 0.7;
	war_fog /= 8.8;
	ret *= mix(0.2, 1.0, war_fog);
#endif
