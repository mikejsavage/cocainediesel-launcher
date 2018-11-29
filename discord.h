#pragma once

enum DiscordState {
	DiscordState_Unauthenticated,
	DiscordState_Authenticating,
	DiscordState_Authenticated,
	DiscordState_Error,
};

struct DiscordUser {
	const char * username;
	const char * discriminator;
	const char * id;
	const char * avatar;
};

void discord_init();
void discord_shutdown();

DiscordState discord_state();
DiscordState discord_update();
void discord_login();
DiscordUser discord_user();
