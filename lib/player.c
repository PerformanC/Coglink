#include <concord/discord.h>

#include <coglink/lavalink-internal.h>
#include <coglink/lavalink.h>

void coglink_playSong(const struct lavaInfo *lavaInfo, char *track, u64snowflake guildId) {
  char payload[2048];
  snprintf(payload, sizeof(payload), "{\"op\":\"play\",\"guildId\":\"%"PRIu64"\",\"track\":\"%s\",\"noReplace\":false,\"pause\":false}", guildId, track);

  __coglink_sendPayload(lavaInfo, payload, sizeof(payload), "play");
}

void coglink_stopPlayer(const struct lavaInfo *lavaInfo, u64snowflake guildID) {
  char payload[64];
  sprintf(payload, "{\"op\":\"stop\",\"guildId\":\"%"PRIu64"\"}", guildID);
  __coglink_sendPayload(lavaInfo, payload, sizeof(payload), "stop");
}

void coglink_pausePlayer(const struct lavaInfo *lavaInfo, u64snowflake guildID, char *pause) {
  char payload[64];
  sprintf(payload, "{\"op\":\"pause\",\"guildId\":\"%"PRIu64"\",\"pause\":%s}", guildID, pause);
  __coglink_sendPayload(lavaInfo, payload, sizeof(payload), "pause");
}

void coglink_seekTrack(const struct lavaInfo *lavaInfo, u64snowflake guildID, char *position) {
  char payload[128];
  sprintf(payload, "{\"op\":\"seek\",\"guildId\":\"%"PRIu64"\",\"position\":%s}", guildID, position);
  __coglink_sendPayload(lavaInfo, payload, sizeof(payload), "seek");
}

void coglink_setPlayerVolume(const struct lavaInfo *lavaInfo, u64snowflake guildID, char *volume) {
  char payload[128];
  sprintf(payload, "{\"op\":\"volume\",\"guildId\":\"%"PRIu64"\",\"volume\":%s}", guildID, volume);
  __coglink_sendPayload(lavaInfo, payload, sizeof(payload), "volume");
}

void coglink_destroyPlayer(const struct lavaInfo *lavaInfo, u64snowflake guildID) {
  char payload[64];
  sprintf(payload, "{\"op\":\"destroy\",\"guildId\":\"%"PRIu64"\"}", guildID);
  __coglink_sendPayload(lavaInfo, payload, sizeof(payload), "destroy");
}
