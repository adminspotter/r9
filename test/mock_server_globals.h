#ifndef __INC_MOCK_SERVER_GLOBALS_H__
#define __INC_MOCK_SERVER_GLOBALS_H__

#include <vector>

#include "../server/classes/zone.h"
#include "../server/classes/action_pool.h"
#include "../server/classes/motion_pool.h"
#include "../server/classes/update_pool.h"
#include "../server/classes/listensock.h"
#include "../server/classes/modules/db.h"
#include "../server/classes/modules/console.h"

int main_loop_exit_flag = 0;
Zone *zone = NULL;
ActionPool *action_pool = NULL;   /* Takes action requests      */
MotionPool *motion_pool = NULL;   /* Processes motion/collision */
UpdatePool *update_pool = NULL;   /* Sends motion updates       */
DB *database = NULL;
std::vector<listen_socket *> sockets;
std::vector<Console *> consoles;

#endif /* __INC_MOCK_SERVER_GLOBALS_H__ */
