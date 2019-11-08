/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   argparse.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dbrophy <dbrophy@student.42.us.org>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2019/11/06 10:51:49 by dbrophy           #+#    #+#             */
/*   Updated: 2019/11/06 10:51:49 by dbrophy          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

void	strmove(char **list, int from, int to)
{
	int		i;
	char	*tmp;

	i = from;
	tmp = list[from];
	while (i > to)
	{
		list[i] = list[i - 1];
		i--;
	}
	list[to] = tmp;
}

int		str_in_arr(char **arr, char *elem)
{
	int i;

	while (*arr)
	{
		i = 0;
		while (elem[i] == (*arr)[i])
			if (elem[i++] == 0)
				return (1);
		arr++;
	}
	return (0);
}

void	argsort(int argc, char **argv, char **dualops)
{
	int	i;

	i = 0;
	while (++i < argc)
	{
		if (str_in_arr(dualops, argv[i]))
		{
			strmove(argv, i, 1);
			if (++i < argc)
				strmove(argv, i, 2);
		}
		else if (argv[i][0] == '-')
			strmove(argv, i, 1);
	}
}

int		get_arg(int argc, char **argv, char *arg)
{
	int i;
	int j;

	j = 0;
	while (++j < argc)
	{
		i = 0;
		while (arg[i] == argv[j][i])
			if (arg[i++] == 0)
				return (j);
	}
	return (-1);
}
