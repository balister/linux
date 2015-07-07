/*
 * Copyright (c) 2015, National Instruments Corp. All rights reserved.
 *
 * Driver for NI Ettus Research USRP E3x0 Daughterboard Regulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/mfd/syscon.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>

static const u32 E3XX_DB_POWER_OFFSET = 28;
static const u32 E3XX_DB_POWER_MASK = BIT(1);
static const u32 E3XX_DB_POWER_ON = BIT(1);
static const u32 E3XX_DB_POWER_OFF = 0;

static int e3xx_db_regulator_enable(struct regulator_dev *rdev)
{
	struct regmap *regmap = rdev_get_drvdata(rdev);
	dev_dbg(&rdev->dev, "Calling regulator enable\n");

	return regmap_update_bits(regmap, E3XX_DB_POWER_OFFSET,
				  E3XX_DB_POWER_MASK, E3XX_DB_POWER_ON);
}

static int e3xx_db_regulator_disable(struct regulator_dev *rdev)
{
	struct regmap *regmap = rdev_get_drvdata(rdev);

	dev_dbg(&rdev->dev, "Calling regulator disable\n");

	return regmap_update_bits(regmap, E3XX_DB_POWER_OFFSET,
				  E3XX_DB_POWER_MASK, E3XX_DB_POWER_OFF);
}

static int e3xx_db_regulator_is_enabled(struct regulator_dev *rdev)
{
	int ret;
	u32 power;
	struct regmap *regmap = rdev_get_drvdata(rdev);

	dev_dbg(&rdev->dev, "Calling regulator is_enabled\n");

	ret = regmap_read(regmap, E3XX_DB_POWER_OFFSET, &power);
	if (ret < 0)
		return ret;

	return !!(power & E3XX_DB_POWER_MASK);
}

static const struct regulator_ops e3xx_db_regulator_ops = {
	.enable =	e3xx_db_regulator_enable,
	.disable =	e3xx_db_regulator_disable,
	.is_enabled =	e3xx_db_regulator_is_enabled,
};

static const struct regulator_desc e3xx_db_regulator_desc = {
	.name = "db-supply",
	.ops = &e3xx_db_regulator_ops,
	.type = REGULATOR_VOLTAGE,
};

static int e3xx_db_regulator_probe(struct platform_device *pdev)
{
	struct regulator_dev *reg_dev;
	struct regulator_init_data *initdata;
	struct regmap *regmap;
	struct regulator_config config = {};

	regmap = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "syscon");
	if (IS_ERR(regmap)) {
		dev_err(&pdev->dev, "unable to get syscon");
		return PTR_ERR(regmap);
	}

	initdata = of_get_regulator_init_data(&pdev->dev, pdev->dev.of_node, NULL);

	config.dev = &pdev->dev;
	config.driver_data = regmap;
	config.init_data = initdata;
	config.of_node = pdev->dev.of_node;

	reg_dev = devm_regulator_register(&pdev->dev,
		&e3xx_db_regulator_desc, &config);
	if (IS_ERR(reg_dev))
		return PTR_ERR(reg_dev);

	return 0;
}

static const struct of_device_id e3xx_db_regulator_match[] = {
	{ .compatible = "ettus,e3xx-db-regulator", },
	{ }
};
MODULE_DEVICE_TABLE(of, e3xx_db_reg_match);

static struct platform_driver e3xx_db_reg_driver = {
	.driver		= {
		.name	= "e3xx-db-reg",
		.of_match_table = of_match_ptr(e3xx_db_regulator_match),
	},
	.probe		= e3xx_db_regulator_probe,
};

module_platform_driver(e3xx_db_reg_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Moritz Fischer <moritz.fischer@ettus.com>");
MODULE_DESCRIPTION("NI Ettus Research USRP E3xx daughterboard regulator driver");
