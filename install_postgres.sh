sudo su postgres -c "kill -INT \`head -1 /usr/local/pgsql/data/postmaster.pid\`"
./configure
make
sudo make install
sudo su postgres -c "/usr/local/pgsql/bin/pg_ctl -D /usr/local/pgsql/data -l ~/logfile start"
# su postgres -c "/usr/local/pgsql/bin/createdb test"
psql -h localhost -U postgres test