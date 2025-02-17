﻿/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

using System.Net;

namespace SequoiaDB
{
    public class ServerAddress
    {
        private const string DEFAULT_HOST = "127.0.0.1";
        private const int DEFAULT_PORT = 11810;

        private IPEndPoint hostAddress;
        private string host;
        private int port;

        public IPEndPoint HostAddress
        {
            get { return hostAddress; }
        }

        public string Host
        {
            get { return host; }
        }

        public int Port
        {
            get { return port; }
        }

        public ServerAddress()
        {
            hostAddress = new IPEndPoint(IPAddress.Parse(DEFAULT_HOST), DEFAULT_PORT);
            this.host = DEFAULT_HOST;
            this.port = DEFAULT_PORT;
        }

        public ServerAddress (string host, int port)
        {
            IPAddress addr = DNSResolve(host);
            hostAddress = new IPEndPoint(addr, port);
            this.host = host;
            this.port = port;        
        }

        public ServerAddress(string host)
        {
            if (host.IndexOf(":") > 0)
            {
                string[] tmp = host.Split(':');
                this.host = tmp[0].Trim();
                this.port = int.Parse(tmp[1].Trim());
            }
            else
            {
                this.host = host;
                this.port = DEFAULT_PORT;
            }
            IPAddress addr = DNSResolve(this.host);
            hostAddress = new IPEndPoint(addr, this.port);
        }

        public ServerAddress(IPEndPoint addr, int port)
        {
            hostAddress = addr;
            hostAddress.Port = port;
            this.host = hostAddress.Address.ToString();
            this.port = hostAddress.Port;
        }

        public ServerAddress(IPEndPoint addr)
        {
            hostAddress = addr;
            this.host = hostAddress.Address.ToString();
            this.port = hostAddress.Port;
        }

        IPAddress DNSResolve(string host)
        {
            try
            {
                IPAddress[] addr = Dns.GetHostAddresses(host);
                return addr[0];
            }
            catch (System.Exception)
            {
                throw new BaseException("SDB_INVALIDARG");
            }
        }
    }
}
