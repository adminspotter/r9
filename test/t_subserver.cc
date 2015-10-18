#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include <gtest/gtest.h>

#define SUBSERVER "../server/r9subserver"
#define ALARM_SECS 10

class SubserverTest : public ::testing::Test
{
  protected:
    pid_t child_pid;
    int child_sock;
    bool close_sock;

    virtual void SetUp()
        {
            int pid, fd[2], ret;

            ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
            ASSERT_EQ(ret, 0);

            if ((pid = fork()) != 0)
            {
                /* The parent.  We'll use fd[0]. */
                ret = close(fd[1]);
                ASSERT_EQ(ret, 0);
                this->child_sock = fd[0];
                this->child_pid = pid;
            }
            else if (pid == 0)
            {
                struct rlimit rlim;
                int i;

                /* The child.  We'll use fd[1]. */
                ret = dup2(fd[1], STDIN_FILENO);
                ASSERT_EQ(ret, STDIN_FILENO);

                /* Close everything else */
                ret = getrlimit(RLIMIT_NOFILE, &rlim);
                ASSERT_EQ(ret, 0);
                for (i = 0; i < rlim.rlim_cur; ++i)
                    if (i != STDIN_FILENO)
                        close(i);

                ret = execl(SUBSERVER, (char *)NULL);

                /* If this returns, go ahead and blow up */
                ASSERT_EQ(ret, 0);
            }
            else
                /* Failed to fork, so go ahead and bomb */
                ASSERT_EQ(pid, 0);
        };

    virtual void TearDown()
        {
            int ret;

            if (!this->close_sock)
            {
                ret = kill(this->child_pid, SIGTERM);
                ASSERT_EQ(ret, 0);
                ret = waitpid(this->child_pid, NULL, 0);
                ASSERT_EQ(ret, this->child_pid);
            }
            else
                alarm(ALARM_SECS);
            ret = close(this->child_sock);
            EXPECT_EQ(ret, 0);
            if (this->close_sock)
            {
                ret = waitpid(this->child_pid, NULL, 0);
                if (ret == -1)
                    ASSERT_NE(errno, EINTR);
                else
                    ASSERT_EQ(ret, this->child_pid);
                alarm(0);
            }
        };

    virtual int pass_fd(int new_fd)
        {
            struct cmsghdr cmptr;
            struct iovec iov[1];
            struct msghdr msg;
            char buf[2];

            /* We will use the BSD4.4 method, since Linux uses that method */
            iov[0].iov_base = buf;
            iov[0].iov_len = 2;
            msg.msg_iov = iov;
            msg.msg_iovlen = 1;
            msg.msg_name = NULL;
            msg.msg_namelen = 0;
            if (new_fd < 0)
            {
                msg.msg_control = NULL;
                msg.msg_controllen = 0;
                buf[1] = -this->child_sock;
                if (buf[1] == 0)
                    buf[1] = 1;
            }
            else
            {
                cmptr.cmsg_level = SOL_SOCKET;
                cmptr.cmsg_type = SCM_RIGHTS;
                cmptr.cmsg_len = sizeof(struct cmsghdr) + sizeof(int);
                msg.msg_control = (caddr_t)(&cmptr);
                msg.msg_controllen = sizeof(struct cmsghdr) + sizeof(int);
                *(int *)CMSG_DATA((&cmptr)) = new_fd;
                buf[1] = 0;
            }
            buf[0] = 0;
            if (sendmsg(this->child_sock, &msg, 0) != 2)
                return -1;
            return 0;
        };
};

TEST_F(SubserverTest, CloseFD)
{
    this->close_sock = true;
}

TEST_F(SubserverTest, PassFD)
{
    int ret, fd[2];

    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    ASSERT_EQ(ret, 0);

    /* We'll use fd[0] and pass in fd[1] */
    ret = this->pass_fd(fd[1]);
    ASSERT_EQ(ret, 0);

    /* Not sure how to test whether the socket is still live. */

    this->close_sock = false;
}

/*TEST_F(SubserverTest, ReceivePacket)
{
    int ret, fd[2];

    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    ASSERT_EQ(ret, 0);

    /* We'll use fd[0] and pass in fd[1] *
    ret = this->pass_fd(fd[1]);
    ASSERT_EQ(ret, 0);
    ret = close(fd[1]);
    ASSERT_EQ(ret, 0);

    /* Spew some stuff into fd[0] and see what comes out of child_sock *
    fprintf(fd[0], "hey, this is some stuff");

    this->close_sock = false;
}
*/
