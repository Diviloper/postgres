sudo su postgres -c "kill -INT \`head -1 /usr/local/pgsql/data2/postmaster.pid\`"
make
sudo make install
sudo su postgres -c "/usr/local/pgsql/bin/pg_ctl -D /usr/local/pgsql/data2 start"
# su postgres -c "/usr/local/pgsql/bin/createdb test"
psql -h localhost -U postgres test -c "VACUUM ANALYZE temp;"
psql -h localhost -U postgres test -c "VACUUM ANALYZE temp2;"
psql -h localhost -U postgres test